#include "../../include/tlk_util.h"

/*
 * Parse network-ordered port number from @input string and put it in @output
 * Returns 0 on success, -1 on failure
 */
int parse_port_number (const char *input, unsigned short *output) {
  long tmp;

  tmp = strtol(input, NULL, 0);

  if (tmp < 1024 || tmp > 49151) {
    return -1;
  }

  *output = htons((unsigned short) tmp);

  return 0;
}

/*
 * Print usage (server version)
 */
void usage_error_server (const char *prog_name) {
  fprintf(stderr, "Usage: %s <port_number>\n", prog_name);
  exit(EXIT_FAILURE);
}

/*
 * Print usage (client version)
 */
void usage_error_client (const char *prog_name) {
  fprintf(stderr, "Usage: %s <IP_address> <port_number> <nickname>\n", prog_name);
  exit(EXIT_FAILURE);
}

/*
 * Free @user socket and data structure
 * Exit thread on return
 */
void close_and_free_chat_session (tlk_user_t *user) {
  fprintf(stderr, "User connection unexpectedly closed\n");

  int ret = tlk_socket_close(user -> socket);
  ERROR_HELPER(ret, "Cannot close user socket");

  free(user -> address);
  free(user);
  tlk_thread_exit(NULL);
}

/*
 * Send @msg on @socket
 * Returns the number of bytes sent on success, TLK_SOCKET_ERROR on failure
 */
int send_msg (tlk_socket_t socket, const char *msg) {
  int ret;
  char msg_to_send[MSG_SIZE];

  snprintf(
    msg_to_send,
    strlen(msg) + 1,
    "%s", msg
  );

  int bytes_left = strlen(msg_to_send);
  int bytes_sent = 0;

  while (bytes_left > 0) {
      ret = send(socket, msg_to_send + bytes_sent, bytes_left, 0);

      if (ret == TLK_SOCKET_ERROR) {
        if (errno == TLK_EINTR)
          continue;
        return TLK_SOCKET_ERROR;
      }

      bytes_left -= ret;
      bytes_sent += ret;
  }
  ret = send(socket, "\n", 1, 0);

  return ret;
}

/*
 * Receive @buf_len bytes from @socket and put contents inside @buf
 * Returns:
 *  Number of bytes read on success
 *  TLK_CONN_CLOSED if the endpoint has gracefully closed the connection
 *  TLK_SOCKET_ERROR on failure
 */
int recv_msg (tlk_socket_t socket, char *buf, int buf_len) {
  int ret;
  int bytes_read = 0;

  while (bytes_read <= buf_len) {
      ret = recv(socket, buf + bytes_read, 1, 0);

      if (ret == 0) return TLK_CONN_CLOSED;
      if (ret == TLK_SOCKET_ERROR) {
        if (errno == TLK_EINTR)
          continue;
        return TLK_SOCKET_ERROR;
       }

      if (buf[bytes_read] == '\n') break;

      bytes_read++;
  }

  buf[bytes_read] = '\0';
  return bytes_read;
}

/*
 * Parse nickname from join message
 * Returns 0 on success, -1 on failure
 */
char join_msg_prefix[MSG_SIZE];
size_t join_msg_prefix_len = 0;

int parse_join_msg (char *msg, size_t msg_len, char *nickname) {

  /* Build message prefix only the first time for efficiency */
  if (join_msg_prefix_len == 0) {

    snprintf(
      join_msg_prefix,
      strlen("/join "),
      "%c%s ",
      COMMAND_CHAR, JOIN_COMMAND
    );
    join_msg_prefix_len = strlen(join_msg_prefix);
  }

  int ret = strncmp(msg, join_msg_prefix, join_msg_prefix_len);

  if (msg_len > join_msg_prefix_len && ret == 0) {

    snprintf(
      nickname,
      msg_len - (join_msg_prefix_len) + 1,
      "%s",
      msg + join_msg_prefix_len + 1
    );
    return 0;
  } else {
    return -1;
  }
}

/*
 * Send help message and commands list through @socket
 * Returns nothing
 */
void send_help (tlk_socket_t socket) {

  char msg[MSG_SIZE];

  /* Send help message */
  send_msg(socket, "\n");
  send_msg(socket, "Commands List\n");


  /* Send commands list */
  snprintf(
    msg,
    strlen(HELP_MSG) + sizeof(COMMAND_CHAR) + strlen(HELP_COMMAND) + 1,
    HELP_MSG,
    COMMAND_CHAR, HELP_COMMAND
  );
  send_msg(socket, msg);

  snprintf(
    msg,
    strlen(LIST_MSG) + sizeof(COMMAND_CHAR) + strlen(LIST_COMMAND) + 1,
    LIST_MSG,
    COMMAND_CHAR, LIST_COMMAND
  );
  send_msg(socket, msg);

  snprintf(
    msg,
    strlen(TALK_MSG) + sizeof(COMMAND_CHAR) + strlen(TALK_COMMAND) + 1,
    TALK_MSG,
    COMMAND_CHAR, TALK_COMMAND
  );
  send_msg(socket, msg);

  snprintf(
    msg,
    strlen(CLOSE_MSG) + sizeof(COMMAND_CHAR) + strlen(CLOSE_COMMAND) + 1,
    CLOSE_MSG,
    COMMAND_CHAR, CLOSE_COMMAND
  );
  send_msg(socket, msg);

  snprintf(
    msg,
    strlen(QUIT_MSG) + sizeof(COMMAND_CHAR) + strlen(QUIT_COMMAND) + 1,
    QUIT_MSG,
    COMMAND_CHAR, QUIT_COMMAND
  );
  send_msg(socket, msg);

}

/*
 * Send users @list through @socket as a list of nicknames
 * Returns nothing
 */
void send_users_list (tlk_socket_t socket, tlk_user_t *list[MAX_USERS]) {

  int i;

  /* Send users list, nickname per nickname */
  tlk_user_t *aux;
  send_msg(socket, "Users list\n");
  for (i = 0; i < MAX_USERS; i++) {
    aux = list[i];
    if (aux != NULL) {
      send_msg(socket, aux -> nickname);
    }
  }

}

tlk_message_t *talk_session (tlk_user_t *user, char msg[MSG_SIZE]) {

  char error_msg[MSG_SIZE];
/*
  size_t msg_len = strlen(msg);
  size_t talk_command_len = strlen(TALK_COMMAND) + 1;

  if (msg_len <= talk_command_len + 1) {

    if (LOG) printf("\n\t*** [USR] Error no nickname specified\n\n");
    snprintf(error_msg, strlen(NO_NICKNAME), NO_NICKNAME);

    send_msg(user -> socket, error_msg);
    return NULL;
  }

  (user -> listener) = tlk_user_find(msg + talk_command_len + 1);*/
  if ((user -> listener) == NULL) {

    return NULL;

  } else {

    if ((user -> listener) -> status == IDLE) {

      /* Start talking with listener */
      user -> status = TALKING;
      (user -> listener) -> status = TALKING;
      (user -> listener) -> listener = user;

      /* Send an information message */
      snprintf(error_msg, strlen(BEGIN_CHAT_MSG) + strlen(user -> nickname), BEGIN_CHAT_MSG, user -> nickname);

      tlk_message_t *tlk_msg = (tlk_message_t *) malloc(sizeof(tlk_message_t));

      tlk_msg -> id         = (user -> listener) -> id;
      tlk_msg -> sender     = user;
      tlk_msg -> receiver   = (user -> listener);

      tlk_msg -> content    = (char *) calloc(MSG_SIZE, sizeof(char));

      sprintf(tlk_msg -> content, error_msg);
      return tlk_msg;

    } else {
      return NULL;
    }
  }

  return NULL;
}
