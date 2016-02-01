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
 * Returns nothing
 */
void send_msg (tlk_socket_t socket, const char *msg) {
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

      if (ret == -1 && errno == TLK_EINTR) continue;
      ERROR_HELPER(ret, "Cannot write on socket");

      bytes_left -= ret;
      bytes_sent += ret;
  }
  ret = send(socket, "\n", 1, 0);
  ERROR_HELPER(ret, "Cannot write on socket");
}

/*
 * Receive @buf_len bytes from @socket and put contents inside @buf
 * Returns true bytes read
 */
int recv_msg (tlk_socket_t socket, char *buf, size_t buf_len) {
  int ret;
  int bytes_read = 0;

  while (bytes_read <= buf_len) {
      ret = recv(socket, buf + bytes_read, 1, 0);

      if (ret == 0) return -1;
      if (ret == -1 && errno == TLK_EINTR) continue;
      ERROR_HELPER(ret, "Errore nella lettura da socket");

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

    snprintf(nickname, msg_len, "%s", msg + join_msg_prefix_len + 1);
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

  int i, ret;
  char msg[MSG_SIZE];

  send_msg(socket, "Users list\n");

  /* Send users list, nickname per nickname */
  tlk_user_t *aux;
  for (i = 0; i < MAX_USERS; i++)
  {
    if (list[i] != NULL)
    {
      aux = list[i] -> nickname;
      snprintf(msg, strlen(aux) + 1, "%s\n", aux);
      send_msg(socket, msg);
    }
  }

}

/*
 *
 *
 */
char talk_msg_prefix[MSG_SIZE];
size_t talk_msg_prefix_len = 0;

char *parse_talkmsg_target(char msg[MSG_SIZE]) {

  int ret;
  char *nickname[NICKNAME_SIZE];
  size_t msg_len = strlen(msg);

  /* Build message prefix only the first time for efficiency */
/*  printf("\n\nBuild msg prefix only first time\n\n");
  if (talk_msg_prefix_len == 0) {
    printf("\n\nfirst time\n\n");
*/
/*
    snprintf(
      talk_msg_prefix,
      strlen(COMMAND_CHAR) + strlen(TALK_COMMAND) + 1,
      "%c%s ",
      COMMAND_CHAR, TALK_COMMAND
    );


    printf("\n\ntalk_msg_prefix: %s\n\n", talk_msg_prefix);

    talk_msg_prefix_len = strlen(talk_msg_prefix);
  }
*/
  ret = strncmp(msg, "/talk", strlen("/talk"));
  printf("\n\nstrncmp(msg, talk_msg, prefix) -> %d\n\n", ret);

  if (msg_len > talk_msg_prefix_len && ret == 0) {
    printf("\n\nmsg_len > talk_msg_prefix_len && ret == 0\n\n");

    snprintf(nickname, msg, "%s", msg + strlen("/talk") + 1);

printf("\n\nafter snprintf: %s\n\n", nickname);
    return nickname;

  } else {

    return NULL;

  }

}
