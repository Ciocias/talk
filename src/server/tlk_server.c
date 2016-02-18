#include "../../include/tlk_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int incremental_id = 1;

/* Users handling data */
tlk_sem_t users_mutex;
unsigned int current_users;
tlk_user_t *users_list[MAX_USERS];

/* Input queue for broker thread */
tlk_queue_t *waiting_queue          = NULL;

/*
 * Print server usage to standard error
 * Returns nothing
 */
void usage_error_server (const char *prog_name) {
  fprintf(stdout, "Usage: %s <port_number>\n", prog_name);
  exit(EXIT_FAILURE);
}

/* Initialize server data */
unsigned short initialize_server (const char *argv[]) {

  int ret;
  unsigned short port = 0;

  ret = parse_port_number(argv[1], &port);
  if (ret == -1) {
    fprintf(stdout, "Port not valid: must be between 1024 and 49151\n");
    exit(EXIT_FAILURE);
  }

  /* Initialize user data semaphore */
  ret = tlk_sem_init(&users_mutex, 1, 1);
  if (ret) {
    if (LOG) fprintf(stderr, "Cannot initialize users_mutex semaphore\n");
    exit(EXIT_FAILURE);
  }

  /* Initialize global current_users */
  current_users = 0;

  /* Initialize global waiting_queue */
  waiting_queue = tlk_queue_new(QUEUE_SIZE);
  if (waiting_queue == NULL) {
    if (LOG) fprintf(stderr, "Cannot create new waiting_queue with size %d\n", QUEUE_SIZE);
    exit(EXIT_FAILURE);
  }

#if defined(_WIN32) && _WIN32
  /* Initialize Winsock DLL */
  ret = tlk_socket_init();
  if (ret == TLK_SOCKET_ERROR) {
	  if (LOG) fprintf(stderr, "Cannot initialize Winsock DLL\n");
	  exit(EXIT_FAILURE);
  }
#endif

  return port;
}

/* Server listening loop */
void server_main_loop (unsigned short port_number) {

  int ret;
  tlk_socket_t server_desc, client_desc;

  /* Set up socket and other connection data */
  struct sockaddr_in server_addr = { 0 };
  int sockaddr_len = sizeof(struct sockaddr_in);

  server_desc = tlk_socket_create(AF_INET, SOCK_STREAM, 0);
  if (server_desc == TLK_SOCKET_INVALID) {
	  if (LOG) fprintf(stderr, "Cannot create socket\n");
	  exit(EXIT_FAILURE);
  }

  server_addr.sin_addr.s_addr   = INADDR_ANY;
  server_addr.sin_family        = AF_INET;
  server_addr.sin_port          = port_number;

  int reuseaddr_opt = 1;

  ret = setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
  if (ret) {
	  if (LOG) fprintf(stderr, "Cannot set socket option SO_REUSEADDR\n");
	  exit(EXIT_FAILURE);
  }

  ret = tlk_socket_bind(server_desc, (struct sockaddr *) &server_addr, sockaddr_len);
  if (ret == TLK_SOCKET_ERROR) {
	  if (LOG) fprintf(stderr, "Cannot bind socket\n");
	  exit(EXIT_FAILURE);
  }

  ret = tlk_socket_listen(server_desc, MAX_CONN_QUEUE);
  if (ret == TLK_SOCKET_ERROR) {
	  if (LOG) fprintf(stderr, "Cannot listen on socket\n");
	  exit(EXIT_FAILURE);
  }

  struct sockaddr_in *client_addr = (struct sockaddr_in *) calloc(1, sizeof(struct sockaddr_in));

  /* Wait for incoming connections */
  while (1)
  {
    client_desc = tlk_socket_accept(server_desc, (struct sockaddr *) client_addr, sockaddr_len);

    if (client_desc == TLK_SOCKET_INVALID) {
		if (TLK_SOCKET_ERRNO == TLK_EINTR) continue;

		if (LOG) fprintf(stderr, "Cannot accept on socket\n");
		exit(EXIT_FAILURE);
	}

    /* Create new user */
    tlk_user_t *new_user;

    ret = tlk_sem_wait(&users_mutex);
    if (ret) {
      if (LOG) fprintf(stderr, "Cannot wait on users_mutex semaphore\n");
      exit(EXIT_FAILURE);
    }

    new_user = tlk_user_new(incremental_id, client_desc, client_addr);
    if (new_user == NULL) {
       if (LOG) fprintf(stderr, "Cannot create new user");
      exit(EXIT_FAILURE);
    }

    incremental_id += 1;

    ret = tlk_sem_post(&users_mutex);
    if (ret) {
      if (LOG) fprintf(stderr, "Cannot post on users_mutex semaphore\n");
      exit(EXIT_FAILURE);
    }

    /* Launch user handler thread */
    tlk_thread_t user_thread;

    ret = tlk_thread_create(&user_thread, (tlk_thread_func) user_handler, (tlk_thread_args) new_user);
    if (ret)
    {
      if (LOG) fprintf(stderr, "Error creating a new user thread\n");
      send_msg(client_desc, "Error creating a new user thread\n");
    }

    /* Allocate new space for next user */
    client_addr = (struct sockaddr_in *) calloc(1, sizeof(struct sockaddr_in));
  }
}

/* Broker thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI broker_routine (LPVOID arg)
#elif defined(__linux__) && __linux__

void * broker_routine (void)
#endif
{
  tlk_message_t *msg = (tlk_message_t *) malloc(sizeof(tlk_message_t));

  while (1)
  {
    int ret;

    /* Check for messages in the waiting queue */
    ret = tlk_queue_dequeue(waiting_queue, (void **) &msg);
    if (ret != 0) {
      if (LOG) fprintf(stderr, "[BRK] Cannot check for messages in the waiting queue\n");
      break;
    }


    /* Select correct user queue from @users_list */
    unsigned int i;
    for (i = 0; i < current_users; i++) {

      if (users_list[i] -> id == (msg -> receiver) -> id) {

        /* Sort message in correct queue */
        ret = tlk_queue_enqueue(users_list[i] -> queue, (void **) &msg);
        if (ret != 0) {
          if (LOG) fprintf(stderr, "[BRK] Cannot sort message in correct queue\n");
          break;
        }
      }
    }

  }

  /* Deallocate @msg and exit thread */
  if (msg != NULL) free(msg);
  tlk_thread_exit((tlk_exit_t) EXIT_SUCCESS);

  /* Avoid compiler warnings */
  return (tlk_exit_t) EXIT_SUCCESS;
}

/* User handler thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI user_handler (LPVOID arg)
#elif defined(__linux__) && __linux__

void * user_handler (void *arg)
#endif
{
  int ret;
  char msg[MSG_SIZE];
  tlk_user_t *user = (tlk_user_t *) arg;

  /* Wait for a join message from client */
  int join_msg_len = recv_msg(user -> socket, msg, MSG_SIZE);

  if (join_msg_len < 0 || parse_join_msg(msg, strlen(msg), user -> nickname) != 0) {

    snprintf(msg, strlen(JOIN_FAILED) + 1, "%s", JOIN_FAILED);

    if (LOG) fprintf(stderr, "[USR] Failed to join new user as %s\n", user -> nickname);
    send_msg(user -> socket, msg);

    tlk_user_free(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  /* Register new user with given nickname */
  ret = tlk_user_signin(user);
  if (ret != 0) {

	  if (ret == NICKNAME_ERROR) 
	  {
		snprintf(msg, strlen(NICKNAME_ERROR_MSG) + 1, "%s", NICKNAME_ERROR_MSG);
	  }
	  else if (ret == MAX_USERS_ERROR) 
	  {
		snprintf(msg, strlen(MAX_USERS_ERROR_MSG) + 1, "%s", MAX_USERS_ERROR_MSG);
	  }
	  else 
	  {
		snprintf(msg, strlen(REGISTER_FAILED) + 1, "%s", REGISTER_FAILED);
	  }

    if (LOG) fprintf(stderr, "[USR] Error registering new user as %s: %s\n", user -> nickname, msg);
    send_msg(user -> socket, msg);

    tlk_user_free(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  /* If everything runs fine notify the client */
  snprintf(msg, strlen(JOIN_SUCCESS) + 1, "%s", JOIN_SUCCESS);

  ret = send_msg(user -> socket, msg);
  if (ret == TLK_SOCKET_ERROR) {
    if (LOG) fprintf(stderr, "[USR] Cannot notify the client: delete new user and exit\n");

    tlk_user_signout(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  if (LOG) fprintf(stdout, "@%s is online\n", user->nickname);

  /* Send welcome & help (includes commands list) */
  snprintf(msg, strlen(WELCOME_MSG) + strlen(user -> nickname), WELCOME_MSG, user -> nickname);

  send_msg(user -> socket, msg);
  send_help(user -> socket);

  user_chat_session(user);

  /* Avoid compiler warnings */
  return (tlk_exit_t) NULL;
}

/* User queue-checking thread */
#if defined(_WIN32) && _WIN32

DWORD WINAPI user_receiver (LPVOID arg)
#elif defined(__linux__) && __linux__

void * user_receiver (void *arg)
#endif
{
  int ret;
  tlk_message_t *tlk_msg = (tlk_message_t *) malloc(sizeof(tlk_message_t));
  tlk_user_t *user = (tlk_user_t *) arg;

  while (1)
  {
    /* Check own thread queue (reading) and send to user */
    ret = tlk_queue_dequeue(user -> queue, (void **) &tlk_msg);
    if (ret) 
	{
      if (LOG) fprintf(stderr, "[QCR] Dequeuing error\n");
      tlk_thread_exit((tlk_exit_t) EXIT_FAILURE);
    }

    if (tlk_msg != NULL) 
	{
      if (strncmp(tlk_msg -> content, DIE_MSG, strlen(DIE_MSG)) == 0) 
	  {
	    tlk_thread_exit((tlk_exit_t) EXIT_SUCCESS);
      }

      /* Not a command: send it to our user */
      ret = send_msg(user -> socket, tlk_msg -> content);
      if (ret == TLK_SOCKET_ERROR) 
	  {
        if (LOG) fprintf(stderr, "[QCR] Cannot send message to user\n");
        tlk_thread_exit((tlk_exit_t) EXIT_FAILURE);
      }
    }
  }
}

/* TODO: commands_handler description */
int commands_handler(tlk_user_t *user, char msg[MSG_SIZE]) {

  int ret;
  char error_msg[MSG_SIZE];

  if (strncmp(msg + 1, HELP_CMD, strlen(HELP_CMD)) == 0)
  {

    send_help(user -> socket);

  }
  else if (strncmp(msg + 1, LIST_COMMAND, strlen(LIST_COMMAND)) == 0)
  {

    send_list(current_users, user -> socket, users_list);

  }
  else if (strncmp(msg + 1, TALK_COMMAND, strlen(TALK_COMMAND)) == 0)
  {

    /* TODO: refactor code into subroutine */

    size_t msg_len = strlen(msg);
    size_t talk_command_len = strlen(TALK_COMMAND) + 1;

    /* Send error if no nickname is specified */
    if (msg_len <= talk_command_len + 1)
    {

      if (LOG) fprintf(stderr, "[USR] Error no nickname specified\n");
      snprintf(error_msg, strlen(NO_NICKNAME), NO_NICKNAME);

      send_msg(user -> socket, error_msg);
      return 0;
    }

    /* Try to find user with given nickname */
    user -> listener = tlk_user_find(msg + talk_command_len + 1);
    if (user -> listener == NULL) {
      if (LOG) fprintf(stderr, "[USR] Unable to start new talk session\n");

      snprintf(error_msg, strlen(USER_NOT_FOUND) + 1, USER_NOT_FOUND);

      ret = send_msg(user -> socket, error_msg);
      if (ret == TLK_SOCKET_ERROR) {
        if (LOG) fprintf(stderr, "[USR] Cannot send error_msg to user %s\n", user -> nickname);

        return -1;
      }
    }

    /* Try to start a talking session with the given nickname */
    ret = talk_session(user, waiting_queue);
    if (ret < 0) {
      if (LOG) fprintf(stderr, "[USR] Error enqueuing in waiting queue\n");

      return -1;
    }
  }
  else if(strncmp(msg + 1, QUIT_COMMAND, strlen(QUIT_COMMAND)) == 0 ||
          strncmp(msg + 1, CLOSE_COMMAND, strlen(CLOSE_COMMAND)) == 0)
  {
    if (user -> status == TALKING)
    {
      /*Set both users to IDLE*/
      user -> status = IDLE;
      (user -> listener) -> status = IDLE;

      /* Notify the listener that we're closing connection */
      ret = pack_and_send_msg(
        (user -> listener) -> id,
        user,
        user -> listener,
        (char *) CLOSE_CHAT_MSG,
        waiting_queue
      );
      if (ret)
      {
        if (LOG) fprintf(stderr, "[USR] Error enqueuing in waiting queue\n");

        return -1;
      }

      /* Unset both listeners */
      (user -> listener) -> listener = NULL;
      user -> listener = NULL;
    }
    else
    {

	  if (strncmp(msg + 1, CLOSE_COMMAND, strlen(CLOSE_COMMAND)) == 0)
      {
        ret = pack_and_send_msg(
          user -> id,
          user,
          user,
          (char *) IDLE_MSG,
          waiting_queue
        );
        if (ret)
        {
          if (LOG) fprintf(stderr, "[USR] Cannot send message to user\n");
          return -1;
        }
      }
    } /* (user -> status == TALKING) */

    if (strncmp(msg + 1, QUIT_COMMAND, strlen(QUIT_COMMAND)) == 0)
        {
		  if (LOG) fprintf(stdout, "@%s is offline\n", user->nickname);
          return 1;
        }
  }
  else
  {

    ret = send_unknown(user -> socket);
    if (ret == TLK_SOCKET_ERROR) {
      if (LOG) fprintf(stderr, "[USR] Cannot send error_msg to user %s\n", user -> nickname);
      return -1;
    }

  } /* Unknown command */

  return 0;
}

/* TODO: message_handler description */
int message_handler(tlk_user_t *user, char msg[MSG_SIZE]){
  int ret;

  if (msg[0] == COMMAND_CHAR)
  {
    return commands_handler(user, msg);
  }
  else
  {
    /* Send message to listener if status is TALKING */
    if (user -> status == TALKING)
    {
      if ((user -> listener) != NULL)
      {
        /* Send message through server queue system */
        ret = pack_and_send_msg(
          (user -> listener) -> id,
          user,
          user -> listener,
          msg,
          waiting_queue
        );
        if (ret)
        {
           if (LOG) fprintf(stderr, "[USR] Error enqueuing in waiting queue\n");
           return -1;
        }
      }
      else
      {
        user -> status = IDLE;
        user -> listener = NULL;
      } /* (listener != NULL) */
      return 0;
    } /* (user -> status == TALKING) */
  } /* (msg[0] == COMMAND_CHAR) */
  return 0;
}

/* Chat session handler */
void user_chat_session (tlk_user_t *user) {
  int ret;
  int exit_code;
  int quit = 0;

  char msg[MSG_SIZE];

  /* Launch user_receiver thread */
  tlk_thread_t receiver_thread;

  ret = tlk_thread_create(&receiver_thread, (tlk_thread_func) user_receiver, (tlk_thread_args) user);
  if (ret) 
  {
    if (LOG) fprintf(stderr, "[USR] Cannot create thread 'user_receiver'\n");
    tlk_user_signout(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  do {
    /* Receive data from user */
    int len = recv_msg(user -> socket, msg, MSG_SIZE);

    if (len > 0)
    {
        quit = message_handler(user, msg);
    }
    else if (len < 0)
    {
      /* Client connection unexpectedly closed */
      if (LOG) fprintf(stderr, "[USR] Client connection unexpectedly closed\n");
      quit = -1;
      break;
    }
    else
    {
      /* Ignore empty messages */
    }

  } while (!quit);

  if (quit > 0) {

    /* User choosed to quit */
    sprintf(msg, "%s, Thank you for talking!", user -> nickname);

    send_msg(user -> socket, msg);
  }

  if (user -> status == TALKING) {
    if ((user -> listener) != NULL) {
      (user -> listener) -> status = IDLE;
      (user -> listener) -> listener = NULL;
    }
  }

  /* Terminate user_receiver termination */
  terminate_receiver(user, waiting_queue);
  tlk_user_signout(user);
  tlk_thread_join(&receiver_thread, (void **) &exit_code);

  /* Free resources and close thread */
  tlk_thread_exit((tlk_exit_t) EXIT_SUCCESS);
}

int main (int argc, const char *argv[]) {

  int ret;
  if (argc != 2) {
    usage_error_server(argv[0]);
  }

  /* Initialize server data */
  unsigned short port = initialize_server(argv);

  /* Launch broker thread */
  tlk_thread_t broker_thread;

  ret = tlk_thread_create(&broker_thread, (tlk_thread_func) broker_routine,  NULL);
  if (ret) {
    if (LOG) fprintf(stderr, "Cannot create thread 'broker_thread'\n");
    exit(EXIT_FAILURE);
  }

  /* Listen for incoming connections */
  server_main_loop(port);

#if defined(_WIN32) && _WIN32
  /* Terminates use of the Winsock DLL */
  ret = tlk_socket_cleanup();
  if (ret == TLK_SOCKET_ERROR) {
	  if (LOG) fprintf(stderr, "Cannot terminate use of Winsock DLL\n");
	  exit(EXIT_FAILURE);
  }
#endif

  /* Close Program */
  exit(EXIT_SUCCESS);
}
