#include "../../include/tlk_util.h"

/*
 * Parse network-ordered port number from @input string and put it in @output
 * Returns 0 on success, -1 on failure
 */
int parse_port_number(const char *input, unsigned short *output) {
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
void usage_error_server(const char *prog_name) {
  fprintf(stderr, "Usage: %s <port_number>\n", prog_name);
  exit(EXIT_FAILURE);
}

/*
 * Print usage (client version)
 */
void usage_error_client(const char *prog_name) {
  fprintf(stderr, "Usage: %s <IP_address> <port_number>\n", prog_name);
  exit(EXIT_FAILURE);
}

/*
 * Free @user socket and data structure
 * Exit thread on return
 */
void close_and_free_chat_session(tlk_user_t *user) {
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
void send_msg(tlk_socket_t socket, const char *msg) {
  int ret;
  char msg_to_send[MSG_SIZE];

  snprintf(msg_to_send, strlen(msg), "%s", msg);

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
size_t recv_msg(tlk_socket_t socket, char *buf, size_t buf_len) {
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

    snprintf(join_msg_prefix, strlen("/join "), "%c%s ", COMMAND_CHAR, JOIN_COMMAND);
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
void send_help(tlk_socket_t socket) {

  char msg[MSG_SIZE];

  /* Send help message */
/*  snprintf(msg, 14, "Commands list:"); */
  send_msg(socket, "Commands List:\n");

  /* Send commands list */
  snprintf(msg, strlen(HELP_MSG) + sizeof(char) + strlen(HELP_COMMAND), HELP_MSG, COMMAND_CHAR, HELP_COMMAND);
  send_msg(socket, msg);

  snprintf(msg, strlen(JOIN_MSG) + sizeof(char) + strlen(JOIN_COMMAND), JOIN_MSG, COMMAND_CHAR, JOIN_COMMAND);
  send_msg(socket, msg);

  snprintf(msg, strlen(LIST_MSG) + sizeof(char) + strlen(LIST_COMMAND), LIST_MSG, COMMAND_CHAR, LIST_COMMAND);
  send_msg(socket, msg);

  snprintf(msg, strlen(TALK_MSG) + sizeof(char) + strlen(TALK_COMMAND), TALK_MSG, COMMAND_CHAR, TALK_COMMAND);
  send_msg(socket, msg);

  snprintf(msg, strlen(QUIT_MSG) + sizeof(char) + strlen(QUIT_COMMAND), QUIT_MSG, COMMAND_CHAR, QUIT_COMMAND);
  send_msg(socket, msg);
}
