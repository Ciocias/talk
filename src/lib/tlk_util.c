#include "../../include/tlk_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * Send @msg on @socket
 * Returns the number of bytes sent on success, TLK_SOCKET_ERROR on failure
 */
int send_msg (tlk_socket_t socket, const char *msg) {
  int ret = 0;
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

  return bytes_sent;
}

/*
 * Receive @buf_len bytes from @socket and put contents inside @buf
 * Returns:
 *  Number of bytes read on success
 *  TLK_CONN_CLOSED if the endpoint has gracefully closed the connection
 *  TLK_SOCKET_ERROR on failure
 */
int recv_msg (tlk_socket_t socket, char *buf, int buf_len) {
  int ret = 0;
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
      strlen(JOIN_COMMAND) + 2,
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
    strlen(HELP_MSG) + sizeof(COMMAND_CHAR) + strlen(HELP_CMD) + 1,
    HELP_MSG,
    COMMAND_CHAR, HELP_CMD
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
void send_list (unsigned int limit, tlk_socket_t socket, tlk_user_t *list[MAX_USERS]) {

  unsigned int i;
  tlk_user_t *aux;

  /* Send users list, nickname per nickname */
  send_msg(socket, "Users list\n");
  for (i = 0; i < limit; i++) {
    aux = list[i];
    if (aux != NULL) {
      send_msg(socket, aux -> nickname);
    }
  }

}

/*
 * Send UNKNOWN_CMD_MSG through @socket
 * Returns nothing
 */
int send_unknown (tlk_socket_t socket) {

  char msg[MSG_SIZE];
  sprintf(msg, UNKNOWN_CMD_MSG, COMMAND_CHAR, HELP_CMD);

  return send_msg(socket, msg);
}

/*
 * Enqueue a DIE_MSG for @user into @queue
 * Returns 0 on success, -1 on failure
 */
int terminate_receiver (tlk_user_t *user, tlk_queue_t *queue) {

  char error_msg[MSG_SIZE];
  snprintf(error_msg, strlen(DIE_MSG) + 1, DIE_MSG);

  return pack_and_send_msg(
    user -> id,
    user,
    user,
    error_msg,
    queue
  );
}

/*
 * Try to establish a talk session
 * Returns:
 *   0 on success,
 *   -1 on enqueuing failure,
 *   1 if listener is NULL or TALKING
 */
int talk_session (tlk_user_t *user, tlk_queue_t *queue) {

  char msg[MSG_SIZE];

  if ((user -> listener) == NULL)
    return 1;

  if ((user -> listener) -> status == IDLE) {

    /* Start talking with listener */
    user -> status = TALKING;
    (user -> listener) -> status = TALKING;
    (user -> listener) -> listener = user;

    /* Send an information message */
    snprintf(msg, strlen(BEGIN_CHAT_MSG) + strlen(user -> nickname), BEGIN_CHAT_MSG, user -> nickname);

    return pack_and_send_msg(
      (user -> listener) -> id,
      user,
      user -> listener,
      msg,
      queue
    );
  }

  return 1;
}

/*
 * Setup a new tlk_message_t with @msg as content and put it in @queue
 * Returns 0 on success, -1 on failure
 */
int pack_and_send_msg (int id, tlk_user_t *sender, tlk_user_t *receiver, char *msg, tlk_queue_t *queue)
{

  tlk_message_t *tlk_msg = (tlk_message_t *) malloc(sizeof(tlk_message_t));

  tlk_msg -> id         = id;
  tlk_msg -> sender     = sender;
  tlk_msg -> receiver   = receiver;

  tlk_msg -> content    = (char *) calloc(MSG_SIZE, sizeof(char));

  sprintf(tlk_msg -> content, msg);

  return tlk_queue_enqueue(queue, (void **) &tlk_msg);
}
