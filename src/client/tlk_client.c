#include "../../include/tlk_client.h"

/* TODO: add sem for thread-safe stdout printing */

int shouldStop = 0;

/* Initialize client data */
tlk_socket_t initialize_client (const char *argv[]) {

  int ret;
  tlk_socket_t socket_desc;
  struct sockaddr_in endpoint_addr = { 0 };

  struct in_addr ip_addr;
  unsigned short port_number;

  /* Parse IP address */
  if (LOG) printf("--> Parse IP address\n");
  ret = inet_pton(AF_INET, argv[1], (void *) &ip_addr);

  if (ret <= 0) {
    if (ret == 0) {
      fprintf(stderr, "Address not valid\n");
    } else if (ret == -1 && errno == EAFNOSUPPORT) {
      fprintf(stderr, "Address family not valid\n");
    }

    exit(EXIT_FAILURE);
  }

  /* Parse port number */
  if (LOG) printf("--> Parse port number\n");
  ret = parse_port_number(argv[2], &port_number);

  if (ret == -1) {
    fprintf(stderr, "Port not valid: must be between 1024 and 49151\n");

    exit(EXIT_FAILURE);
  }

  /* Create socket */
  if (LOG) printf("--> Create socket\n");
  socket_desc = tlk_socket_create(AF_INET, SOCK_STREAM, 0);
  ERROR_HELPER(socket_desc, "Cannot create socket");

  endpoint_addr.sin_addr = ip_addr;
  endpoint_addr.sin_family = AF_INET;
  endpoint_addr.sin_port = port_number;

  /* Connect to given IP on port */
  if (LOG) printf("--> Connecting to server\n");
  ret = tlk_socket_connect(socket_desc, (const struct sockaddr *) &endpoint_addr, sizeof(struct sockaddr_in));
  ERROR_HELPER(ret, "Cannot connect to endpoint");

  return socket_desc;
}

/* Handle chat session */
void chat_session (tlk_socket_t socket, const char *nickname) {

    int ret, exit_code;
    tlk_thread_t chat_threads[2];


    /* Send JOIN_COMMAND to server */
    char join_command[MSG_SIZE];
    int join_command_len = 1 + strlen(JOIN_COMMAND) + strlen(" ") + strlen(nickname) + 1;

    /* Build command message */
    snprintf(
      join_command,
      join_command_len,
      "%c%s %s\n",
      COMMAND_CHAR, JOIN_COMMAND, nickname
    );

    /* Send command to server */
    if (LOG) printf("--> Send JOIN_COMMAND to server\n");
    ret = send_msg(socket, join_command);

    if (ret == TLK_SOCKET_ERROR) {
      if (LOG) printf("--> Error sending JOIN_COMMAND to server\n");
      exit(EXIT_FAILURE);
    }

    /* Check server response for errors */
    int server_res_len;
    char server_res[MSG_SIZE];

    if (LOG) printf("--> Join sent, waiting for response...\n");
    server_res_len = recv_msg(socket, server_res, MSG_SIZE);

    if (server_res_len == TLK_SOCKET_ERROR)
    {

      if (LOG) printf("--> Error reading from server, exiting...\n");
      exit(EXIT_FAILURE);

    } else if (server_res_len == TLK_CONN_CLOSED) {

      if (LOG) printf("--> Server closed the connection, exiting...\n");
      exit(EXIT_FAILURE);

    } else if (strncmp(server_res, JOIN_FAILED, strlen(JOIN_FAILED)) == 0)
    {

      if (LOG) printf("--> Join failed, exiting...\n");
      exit(EXIT_FAILURE);

    } else if (strncmp(server_res, REGISTER_FAILED, strlen(REGISTER_FAILED)) == 0)
    {

      printf("--> Register failed, exiting...\n");
      exit(EXIT_FAILURE);

    } else if (strncmp(server_res, JOIN_SUCCESS, strlen(JOIN_SUCCESS)) != 0)
    {

      if (LOG) printf("--> Server didn't send JOIN_SUCCESS, exiting...\n");
      exit(EXIT_FAILURE);

    } /* Server sent JOIN_SUCCESS! we are now on-line */

    /* Launch receiver thread */
    if (LOG) printf("--> Launch receiver thread\n");
    ret = tlk_thread_create(&chat_threads[0], receiver, (tlk_thread_args *) &socket);
    GENERIC_ERROR_HELPER(ret, ret, "Cannot create receiver thread");

    /* Launch sender thread */
    if (LOG) printf("--> Launch sender thread\n");
    ret = tlk_thread_create(&chat_threads[1], sender, (tlk_thread_args *) &socket);
    GENERIC_ERROR_HELPER(ret, ret, "Cannot create sender thread");

    /* Wait for termination */
    if (LOG) printf("--> Wait for receiver thread termination\n");
    ret = tlk_thread_join(chat_threads[0], &exit_code);
    GENERIC_ERROR_HELPER(ret, exit_code, "Cannot wait for receiver thread termination");

    if (LOG) printf("--> Wait for sender thread termination\n");
    ret = tlk_thread_join(chat_threads[1], &exit_code);
    GENERIC_ERROR_HELPER(ret, exit_code, "Cannot wait for sender thread termination");

    /* Clean resources */
    if (LOG) printf("--> Close socket\n");
    ret = tlk_socket_close(socket);
    ERROR_HELPER(ret, "Cannot close socket");
  }

/* Sender thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI sender (LPVOID arg)
#elif defined(__linux__) && __linux__

void * sender (void *arg)
#endif /* Sender func definition */
{
  if (LOG) printf("\n\t*** [SND] Sender thread running\n\n");

  tlk_socket_t *socket = (tlk_socket_t *) arg;

  /* Set up close comand */
  if (LOG) printf("\n\t*** [SND] Set up close command\n\n");

  int ret;
  char buf[MSG_SIZE];
  char close_command[MSG_SIZE];

  snprintf(close_command, 1 + strlen(QUIT_COMMAND), "%c%s\n", COMMAND_CHAR, QUIT_COMMAND);

  size_t close_command_len = strlen(close_command);

  while (!shouldStop)
  {
    /* Read from stdin */
    if (LOG) printf("\n\t*** [SND] Read from stdin\n\n");

    /* TODO: implement a prompt function */
    printf("--> ");
    if (fgets(buf, sizeof(buf), stdin) != (char *) buf) {
      fprintf(stderr, ">>> Error reading from stdin, exiting... <<<\n");
      exit(EXIT_FAILURE);
    }

    /* Check if endpoint has closed the connection */
    if (LOG) printf("\n\t*** [SND] Check if endpoint has closed connection\n\n");
    if (shouldStop) break;

    /* Send message through socket */
    if (LOG) printf("\n\t*** [SND] Send message through socket\n\n");
    size_t msg_len = strlen(buf);

    ret = send_msg(*socket, buf);

    if (ret == TLK_SOCKET_ERROR) {
      if (LOG) printf("\n\t*** [SND] Error writing to socket\n\n");
      shouldStop = -1;
    }

    /* Check if message was quit command */
    if (msg_len == close_command_len && strncmp(buf, close_command, close_command_len) == 0) {
      if (LOG) printf("\n\t*** [SND] Message was a quit command\n\n");
      shouldStop = 1;
    }
  }

  /* Terminate sender thread */
  if (LOG) printf("\n\t*** [SND] Sender thread termination\n\n");
  tlk_thread_exit(NULL);

  /* Avoid compiler warnings */
  return NULL;
}

/* Receiver thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI receiver (LPVOID arg)
#elif defined(__linux__) && __linux__

void * receiver (void *arg)
#endif /* Receiver func definition */
{
  if (LOG) printf("\n\t*** [REC] Receiver thread running\n\n");

  int ret;
  tlk_socket_t *socket = (tlk_socket_t *) arg;

  /* Set up close command */ /* useful? */
/*
  if (LOG) printf("\n\t*** [REC] Set up close command\n\n");
  char close_command[MSG_SIZE];
  snprintf(close_command, sizeof(char) + strlen(QUIT_COMMAND), "%c%s", COMMAND_CHAR, QUIT_COMMAND);
  size_t close_command_len = strlen(close_command);
*/
  /* Set up timeout interval */
  if (LOG) printf("\n\t*** [REC] Set up timeout interval\n\n");
  fd_set read_descriptors;
  int nfds = *(socket) + 1;

  char buf[MSG_SIZE];
  char delimiter[2];

  snprintf(delimiter, 1, "%c", MSG_DELIMITER_CHAR);

  while (!shouldStop) {

    FD_ZERO(&read_descriptors);
    FD_SET(*socket, &read_descriptors);

    /*
     *  Select an available read descriptor
     *  since we don't use a timeout and block on select(),  there's no need to check if return is 0
     */
    if (LOG) printf("\n\t*** [REC] Select available read descriptor\n\n");
    ret = select(nfds, &read_descriptors, NULL, NULL, NULL);

    if (ret == TLK_SOCKET_ERROR) {
      /* Interrupt received: retry */
      if (errno == TLK_EINTR)
        continue;

      /* Endpoint has closed unexpectedly */
      if (LOG) printf("\n\t*** [REC] Endpoint has closed unexpectedly\n\n");
      shouldStop = -1;
      break;
    }

    /* Read is now possible */
    ret = recv_msg(*socket, buf, MSG_SIZE);
    if (ret == TLK_SOCKET_ERROR) {

      /* Endpoint has closed unexpectedly */
      if (LOG) printf("\n\t*** [REC] Endpoint has closed unexpectedly\n\n");
      shouldStop = -1;
      break;

    } else if (ret == TLK_CONN_CLOSED) {

      /* Endpoint has closed gracefully */
      if (LOG) printf("\n\t*** [REC] Endpoint has gracefully closed\n\n");
      shouldStop = 1;

    } else {

      /* TODO: Let the client know he's talking with someone */

      /* Show received data to user */
      if (LOG) printf("\n\t*** [REC] Show data to user\n\n");
      printf("[Server] %s\n", buf);

    }
  }

  /* Terminate receiver thread */
  if (LOG) printf("\n\t*** [REC] Receiver thread termination\n\n");
  tlk_thread_exit(NULL);

  /* Avoid compiler warnings */
  return NULL;
}

int main (int argc, const char *argv[]) {

  tlk_socket_t socket;

  if (argc != 4) {
    usage_error_client(argv[0]);
  }

  /* Initialize client data */
  if (LOG) printf("\n-> Initialize client data\n\n");
  socket = initialize_client(argv);

  /* Handle chat session */
  if (LOG) printf("\n-> Handle chat session\n\n");
  chat_session(socket, argv[3]);

  /* Close client */
  if (LOG) printf("\n-> Close client\n\n");
  exit(EXIT_SUCCESS);
}
