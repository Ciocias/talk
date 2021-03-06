#include "../../include/client.h"

/* TODO: add thread-safe stdout printing */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int shouldStop = 0;


int main (int argc, const char *argv[]) 
{
  tlk_socket_t socket;

  if (argc != 4)
  {
    /* Print client usage to standard error */
    fprintf(stdout, "Usage: %s <IP_address> <port_number> <nickname>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* Initialize client data */
  socket = initialize_client(argv);

  /* Try to join server as @nickname */
  join_server(&socket, argv[3]);

  /* Handle chat session */
  chat_session(socket);

  #if defined(_WIN32) && _WIN32
  /* Terminates use of the Winsock DLL */
  int ret = tlk_socket_cleanup();
  if (ret == TLK_SOCKET_ERROR)
  {
    if (LOG) fprintf(stderr, "Cannot terminate use of Winsock DLL\n");
    exit(EXIT_FAILURE);
  }
  #endif

  /* Close client */
  exit(EXIT_SUCCESS);
}

/* Initialize client data */
tlk_socket_t initialize_client (const char *argv[]) 
{
  int ret;
  tlk_socket_t socket_desc;
  struct sockaddr_in endpoint_addr = { 0 };

  struct in_addr ip_addr;
  unsigned short port_number;

#if defined(_WIN32) && _WIN32
  /* Initialize Winsock DLL */
  ret = tlk_socket_init();
  if (ret == TLK_SOCKET_ERROR)
  {
	  if (LOG) fprintf(stderr, "Cannot initialize Winsock DLL\n");
	  exit(EXIT_FAILURE);
  }
#endif

  /* Parse IP address */
  ret = inet_pton(AF_INET, argv[1], (void *) &ip_addr);

  if (ret <= 0)
  {
    if (ret == 0)
	  {
      if (LOG) fprintf(stderr, "Address not valid\n");
    }
	  else if (ret == -1 && TLK_SOCKET_ERRNO == TLK_EAFNOSUPPORT)
	  {
      if (LOG) fprintf(stderr, "Address family not valid\n");
    }
    exit(EXIT_FAILURE);
  }

  /* Parse port number */
  ret = parse_port_number(argv[2], &port_number);

  if (ret == -1)
  {
    if (LOG) fprintf(stderr, "Port not valid: must be between 1024 and 49151\n");
    exit(EXIT_FAILURE);
  }

  /* Create socket */
  socket_desc = tlk_socket_create(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == TLK_SOCKET_INVALID)
  {
	  if (LOG) fprintf(stderr, "Cannot create socket\n");
	  exit(EXIT_FAILURE);
  }

  endpoint_addr.sin_addr      = ip_addr;
  endpoint_addr.sin_family    = AF_INET;
  endpoint_addr.sin_port      = port_number;

  /* Connect to given IP on port */
  ret = tlk_socket_connect(socket_desc, (const struct sockaddr *) &endpoint_addr, sizeof(struct sockaddr_in));
  if (ret == TLK_SOCKET_ERROR)
  {
	  if (LOG) fprintf(stderr, "Cannot connect to endpoint\n");
	  tlk_socket_close(socket_desc);
	  exit(EXIT_FAILURE);
  }

  return socket_desc;
}

/* Send JOIN command to server */
void join_server (tlk_socket_t *socket, const char *nickname) 
{
  int ret;
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
  ret = send_msg(*socket, join_command);

  if (ret == TLK_SOCKET_ERROR) 
  {
    if (LOG) fprintf(stderr, "Error sending JOIN_COMMAND to server\n");
    exit(EXIT_FAILURE);
  }

  /* Check server response for errors */
  int server_res_len;
  char server_res[MSG_SIZE];

  server_res_len = recv_msg(*socket, server_res, MSG_SIZE);

  if (server_res_len == TLK_SOCKET_ERROR)
  {
    if (LOG) fprintf(stderr, "Error reading from server socket\n");
    exit(EXIT_FAILURE);
  }
  else if (server_res_len == TLK_CONN_CLOSED)
  {
    if (LOG) fprintf(stderr, "Server closed the connection\n");
    exit(EXIT_FAILURE);
  }
  else if (strncmp(server_res, JOIN_FAILED, strlen(JOIN_FAILED)) == 0)
  {
    if (LOG) fprintf(stderr, "Join failed\n");
    exit(EXIT_FAILURE);
  }
  else if (strncmp(server_res, NICKNAME_ERROR_MSG, strlen(NICKNAME_ERROR_MSG)) == 0)
  {
	  fprintf(stdout, "Nickname already in use\n");
	  exit(EXIT_FAILURE);
  }
  else if (strncmp(server_res, MAX_USERS_ERROR_MSG, strlen(MAX_USERS_ERROR_MSG)) == 0)
  {
	  fprintf(stdout, "Max users limit reached\n");
	  exit(EXIT_FAILURE);
  }
  else if (strncmp(server_res, REGISTER_FAILED, strlen(REGISTER_FAILED)) == 0)
  {
	  if (LOG) fprintf(stderr, "Register failed\n");
	  exit(EXIT_FAILURE);
  }
  else if (strncmp(server_res, JOIN_SUCCESS, strlen(JOIN_SUCCESS)) != 0)
  {
    if (LOG) fprintf(stderr, "Server didn't send JOIN_SUCCESS\n");
    exit(EXIT_FAILURE);
  } /* Server sent JOIN_SUCCESS! we are now on-line */
}

/* Handle chat session */
void chat_session (tlk_socket_t socket) 
{
  int ret;
  int exit_code;
  tlk_thread_t chat_threads[2];

  /* Launch receiver thread */
  ret = tlk_thread_create(&chat_threads[0], receiver, (tlk_thread_args *) &socket);
  if (ret)
  {
    if (LOG) fprintf(stderr, "Cannot create receiver thread\n");
    exit(EXIT_FAILURE);
  }

  /* Launch sender thread */
  ret = tlk_thread_create(&chat_threads[1], sender, (tlk_thread_args *) &socket);
  if (ret)
  {
    if (LOG) fprintf(stderr, "Cannot create sender thread\n");
    exit(EXIT_FAILURE);
  }

  /* Wait for termination */
  ret = tlk_thread_join(&chat_threads[0], (void **) &exit_code);
  if (ret)
  {
    if (LOG) fprintf(stderr, "Cannot wait for receiver thread termination\n");
    exit(EXIT_FAILURE);
  }

  fprintf(stdout, "Press Enter to exit\n");

  ret = tlk_thread_join(&chat_threads[1], (void **) &exit_code);
  if (ret)
  {
    if (LOG) fprintf(stderr, "Cannot wait for sender thread termination\n");
    exit(EXIT_FAILURE);
  }

  /* Clean resources */
  tlk_socket_close(socket);
  return;
}

/* Sender thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI sender (LPVOID arg)
#elif defined(__linux__) && __linux__

void * sender (void *arg)
#endif /* Sender func definition */
{
  int ret;
  tlk_socket_t *socket = (tlk_socket_t *) arg;

  /* Set up close comand */
  char buf[MSG_SIZE];
  char close_command[MSG_SIZE];

  snprintf(close_command, 3 + strlen(QUIT_COMMAND), "%c%s\n", COMMAND_CHAR, QUIT_COMMAND);
  size_t close_command_len = strlen(close_command);

  while (!shouldStop)
  {
    if (shouldStop) break;

    /* Read from stdin */
    /* TODO: implement a prompt function */
    /* fprintf(stdout, "--> "); */
    if (fgets(buf, sizeof(buf), stdin) != (char *) buf)
  	{
      if (LOG) fprintf(stderr, "[SND] Error reading from stdin\n");
      shouldStop = -1;
      tlk_thread_exit((tlk_exit_t) EXIT_FAILURE);
    }

    /* Check if endpoint has closed the connection */
    if (shouldStop) break;

    /* Send message through socket */
    size_t msg_len = strlen(buf);

    ret = send_msg(*socket, buf);

    if (ret == TLK_SOCKET_ERROR)
  	{
      if (LOG) fprintf(stderr, "[SND] Error writing to socket\n");
      shouldStop = -1;
      tlk_thread_exit((tlk_exit_t) EXIT_FAILURE);
    }

    /* Check if message was quit command */
    if ((msg_len == close_command_len) && (strncmp(buf, close_command, close_command_len) == 0))
  	{
      shouldStop = 1;
    }
  }

  /* Terminate sender thread */
  tlk_thread_exit((tlk_exit_t) EXIT_SUCCESS);

  /* Avoid compiler warnings */
  return (tlk_exit_t) EXIT_SUCCESS;
}

/* Receiver thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI receiver (LPVOID arg)
#elif defined(__linux__) && __linux__

void * receiver (void *arg)
#endif /* Receiver func definition */
{
  int ret;
  tlk_socket_t *socket = (tlk_socket_t *) arg;
  /* Close command*/
  char close_command[MSG_SIZE];

  snprintf(close_command, 2 + strlen(QUIT_COMMAND), "%c%s\n", COMMAND_CHAR, QUIT_COMMAND);

  /*size_t close_command_len = strlen(close_command);*/

  /* Die command*/
  char die_command[MSG_SIZE];

  snprintf(die_command, 1 + strlen(DIE_MSG), "%s\n", DIE_MSG);

  size_t die_command_len = strlen(die_command);

  /* Set up read file descriptors */
  fd_set read_descriptors;
  int nfds = *(socket) + 1;

  char buf[MSG_SIZE];

  while (!shouldStop)
  {
    FD_ZERO(&read_descriptors);
    FD_SET(*socket, &read_descriptors);

    /*
     *  Select an available read descriptor
     *  since we don't use a timeout and block on select(),  there's no need to check if return is 0
     */

    if (shouldStop) break;

    ret = select(nfds, &read_descriptors, NULL, NULL, NULL);

    if (ret == TLK_SOCKET_ERROR)
	  {
      /* Interrupt received: retry */
      if (TLK_SOCKET_ERRNO == TLK_EINTR) continue;

      /* Endpoint has closed unexpectedly */
      if (LOG) fprintf(stderr, "[REC] select: Endpoint has closed unexpectedly\n");
      shouldStop = -1;
      tlk_thread_exit((tlk_exit_t) EXIT_FAILURE);
    }

    if (shouldStop) break;

    /* Read is now possible */
    ret = recv_msg(*socket, buf, MSG_SIZE);
    if (ret == TLK_SOCKET_ERROR)
    {
      /* Endpoint has closed unexpectedly */
      if (LOG) fprintf(stderr, "[REC] recv_msg: Endpoint has closed unexpectedly\n");
      shouldStop = -1;
      tlk_thread_exit((tlk_exit_t) EXIT_FAILURE);
    }
    else if (ret == TLK_CONN_CLOSED)
    {
      /* Endpoint has closed gracefully */
      shouldStop = 1;
      break;
    }
    else if ( strlen(buf) == die_command_len && strncmp(buf, die_command, die_command_len) == 0)
    {
      fprintf(stdout, "Server is currently closed\n");

      ret = send_msg(*socket, close_command);
      if (ret == TLK_SOCKET_ERROR)
      {
        if (LOG) fprintf(stderr, "[SND] Error writing to socket\n");
        shouldStop = -1;
        tlk_thread_exit((tlk_exit_t) EXIT_FAILURE);
      }
      shouldStop = 1;
      break;
    }
    else
    {
      /* TODO: Let the client know he's talking with someone */
      /* Show received data to user */
      fprintf(stdout, "%s\n", buf);
    }

  } /* End of while loop */

  /* Terminate receiver thread */
  tlk_thread_exit((tlk_exit_t) EXIT_SUCCESS);

  /* Avoid compiler warnings */
  return (tlk_exit_t) EXIT_SUCCESS;
}
