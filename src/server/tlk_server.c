#include "../../include/tlk_server.h"

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
    fprintf(stderr, "Port not valid: must be between 1024 and 49151\n");
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
  ERROR_HELPER(server_desc, "Cannot create socket");

  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_family      = AF_INET;
  server_addr.sin_port        = port_number;

  int reuseaddr_opt = 1;

  if (LOG) fprintf(stderr, "Set SO_REUSEADDR option\n");
  ret = setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
  ERROR_HELPER(ret, "Cannot set option SO_REUSEADDR");

  if (LOG) fprintf(stderr, "Bind on socket\n");
  ret = tlk_socket_bind(server_desc, (struct sockaddr *) &server_addr, sockaddr_len);
  ERROR_HELPER(ret, "Cannot bind on socket");

  if (LOG) fprintf(stderr, "Listen on socket\n");
  ret = tlk_socket_listen(server_desc, MAX_CONN_QUEUE);
  ERROR_HELPER(ret, "Cannot listen on socket");

  struct sockaddr_in *client_addr = (struct sockaddr_in *) calloc(1, sizeof(struct sockaddr_in));

  /* Wait for incoming connections */
  if (LOG) printf("Enter 'wait for incoming connections' loop\n");
  while (1)
  {
    client_desc = tlk_socket_accept(server_desc, (struct sockaddr *) client_addr, sockaddr_len);

    if (LOG) fprintf(stderr, "Someone connected\n");

    if (client_desc == -1 && errno == TLK_EINTR) continue;
    ERROR_HELPER(client_desc, "Cannot accept on socket");

    /* Create new user */
    if (LOG) fprintf(stderr, "Create new user\n");
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
    if (LOG) fprintf(stderr, "Launch user handler thread\n");
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

void * broker_routine (void *arg)
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

  if (LOG) printf("\n\t*** [USR] User handler thread running\n\n");

  /* Wait for a join message from client */
  if (LOG) printf("\n\t*** [USR] Wait for a join message from client\n\n");
  int join_msg_len = recv_msg(user -> socket, msg, MSG_SIZE);

  if (join_msg_len < 0 || parse_join_msg(msg, strlen(msg), user -> nickname) != 0) {

    snprintf(msg, strlen(JOIN_FAILED) + 1, "%s", JOIN_FAILED);

    if (LOG) printf("\n\t*** [USR] Failed to join new user as %s\n\n", user -> nickname);
    send_msg(user -> socket, msg);

    tlk_user_free(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  /* Register new user with given nickname */
  if (LOG) printf("\n\t*** [USR] Register new user as %s\n\n", user -> nickname);
  ret = tlk_user_signin(user);
  if (ret != 0) {

    snprintf(msg, strlen(REGISTER_FAILED) + 1, "%s", REGISTER_FAILED);

    if (LOG) printf("\n\t*** [USR] Failed to register new user as %s\n\n", user -> nickname);
    send_msg(user -> socket, msg);

    tlk_user_free(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  /* If everything runs fine notify the client */
  snprintf(msg, strlen(JOIN_SUCCESS) + 1, "%s", JOIN_SUCCESS);

  if (LOG) printf("\n\t*** [USR] Successfully registered new user %s!\n\n", user -> nickname);
  ret = send_msg(user -> socket, msg);
  if (ret == TLK_SOCKET_ERROR) {
    if (LOG) printf("\n\t*** [USR] Cannot notify the client: delete new user and exit...\n\n");

    tlk_user_signout(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  /* Send welcome & help (includes commands list) */
  if (LOG) printf("\n\t*** [USR] Sending welcome and help\n\n");

  snprintf(msg, strlen(WELCOME_MSG) + strlen(user -> nickname), WELCOME_MSG, user -> nickname);

  send_msg(user -> socket, msg);
  send_help(user -> socket);

  if (LOG) printf("\n\t*** [USR] User chat session started\n\n");
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
    if (LOG) printf("\n\t*** [QCR] Check own thread queue\n\n");
    ret = tlk_queue_dequeue(user -> queue, (void **) &tlk_msg);
    if (ret) {
      if (LOG) printf("\n\t*** [QCR] Dequeuing error\n\n\n");
      tlk_thread_exit((tlk_exit_t) NULL);
    }

    if (tlk_msg != NULL) {
      if (strncmp(tlk_msg -> content, DIE_MSG, strlen(DIE_MSG)) == 0) {
				printf("\n\t*** [QCR] Die message received\n\n\n");
				tlk_thread_exit((tlk_exit_t) NULL);
      }

      /* Not a command: send it to our user */
      ret = send_msg(user -> socket, tlk_msg -> content);
      if (ret == TLK_SOCKET_ERROR) {
        if (LOG) printf("\n\t*** [QCR] Cannot send message to user\n\n\n");
        tlk_thread_exit((tlk_exit_t) NULL);
      }
    }
  }
}

/* TODO: commands_handler description */
int commands_handler(tlk_user_t *user, tlk_queue_t *queue, char msg[MSG_SIZE]) {

  int ret;
  char error_msg[MSG_SIZE];

  if (strncmp(msg + 1, HELP_CMD, strlen(HELP_CMD)) == 0)
  {

    if (LOG) printf("\n\t*** [USR] User asked for help\n\n");
    send_help(user -> socket);

  }
  else if (strncmp(msg + 1, LIST_COMMAND, strlen(LIST_COMMAND)) == 0)
  {

    if (LOG) printf("\n\t*** [USR] User asked the list\n\n");
    send_list(current_users, user -> socket, users_list);

  }
  else if (strncmp(msg + 1, TALK_COMMAND, strlen(TALK_COMMAND)) == 0)
  {

    if (LOG) printf("\n\t*** [USR] User asked to talk\n\n");

    /* TODO: refactor code into subroutine */

    size_t msg_len = strlen(msg);
    size_t talk_command_len = strlen(TALK_COMMAND) + 1;

    /* Send error if no nickname is specified */
    if (msg_len <= talk_command_len + 1) {

      if (LOG) printf("\n\t*** [USR] Error no nickname specified\n\n");
      snprintf(error_msg, strlen(NO_NICKNAME), NO_NICKNAME);

      send_msg(user -> socket, error_msg);
      return 0;
    }

    /* Try to find user with given nickname */
    user -> listener = tlk_user_find(msg + talk_command_len + 1);
    if (user -> listener == NULL) {
      if (LOG) printf("\n\t*** [USR] Unable to start new talk session \n\n");

      snprintf(error_msg, strlen(USER_NOT_FOUND) + 1, USER_NOT_FOUND);

      ret = send_msg(user -> socket, error_msg);
      if (ret == TLK_SOCKET_ERROR) {
        if (LOG) printf("\n\t*** [USR] Cannot send error_msg to user %s\n\n", user -> nickname);

        terminate_receiver(user, queue);
        return -1;
      }

    }

    /* Try to start a talking session with the given nickname */
    ret = talk_session(user, queue);
    if (ret < 0) {
      if (LOG) printf("\n\t*** [USR] Error enqueuing in waiting queue\n\n\n");

      terminate_receiver(user, queue);
      return -1;
    }
  }
  else if (strncmp(msg + 1, QUIT_COMMAND, strlen(QUIT_COMMAND)) == 0)
  {

    if (LOG) printf("\n\t*** [USR] User asked to quit\n\n");
    if (user -> status == TALKING) return 0;

    terminate_receiver(user, queue);
    return -1;
  }
  else if(strncmp(msg + 1, CLOSE_COMMAND, strlen(CLOSE_COMMAND)) == 0)
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
        (char *) CLOSE_TALK_MSG,
        waiting_queue
      );
      if (ret) {
        if (LOG) printf("\n\t*** [USR] Error enqueuing in waiting queue\n\n\n");

        terminate_receiver(user, queue);
        return -1;
      }

      /* Unset both listeners */
      (user -> listener) -> listener = NULL;
      user -> listener = NULL;
    }
    else
    {
      sprintf(error_msg, IDLE_MSG);

      ret = send_msg(user -> socket, error_msg);
      if (ret == TLK_SOCKET_ERROR) {
        if (LOG) printf("\n\t*** [USR] Cannot send message to user\n\n\n");

        terminate_receiver(user, queue);
        return -1;
      }
    } /* (user -> status == TALKING) */
  }
  else
  {

    if (LOG) printf("\n\t*** [USR] User didn't ask\n\n");
    ret = send_unknown(user -> socket);
    if (ret == TLK_SOCKET_ERROR) {
      if (LOG) printf("\n\t*** [USR] Cannot send error_msg to user %s\n\n", user -> nickname);

      terminate_receiver(user, queue);
      return -1;
    }

  } /* Unknown command */

  return 0;
}

/* Chat session handler */
void user_chat_session (tlk_user_t *user) {
  int ret, exit_code;
  int quit = 0;

  char msg[MSG_SIZE];

  /* Launch user_receiver thread */
  tlk_thread_t receiver_thread;
  if (LOG) fprintf(stderr, "Launch %s queue-checking routine thread\n\n", user->nickname);

	ret = tlk_thread_create(&receiver_thread, (tlk_thread_func) user_receiver, (tlk_thread_args) user);
	if (ret) {
    if (LOG) fprintf(stderr, "\n\t*** [USR] Cannot create thread 'user_receiver'\n\n\n");
    tlk_user_signout(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  do {
    /* Receive data from user */
    if (LOG) printf("\n\t*** [USR] Receive data from user\n\n");
    int len = recv_msg(user -> socket, msg, MSG_SIZE);

    if (len > 0)
    {

      /* Handle server commands */
      if (msg[0] == COMMAND_CHAR)
      {

        if (LOG) printf("\n\t*** [USR] Handle server commands\n\n");
        quit = commands_handler(user, waiting_queue, msg);
      }
      else
      {

        /* Send message to listener if status is TALKING */
        if (user -> status == TALKING) {
          if ((user -> listener) != NULL) {

            /* Send message through server queue system */
            ret = pack_and_send_msg(
              (user -> listener) -> id,
              user,
              user -> listener,
              msg,
              waiting_queue
            );
            if (ret) {
              if (LOG) printf("\n\t*** [USR] Error enqueuing in waiting queue\n\n\n");
              quit = -1;
              break;
            }
          } else {
            user -> status = IDLE;
            user -> listener = NULL;
          } /* (listener != NULL) */
        } /* (user -> status == TALKING) */
      } /* (msg[0] == COMMAND_CHAR) */
    }
    else if (len < 0)
    {

      /* Client connection unexpectedly closed */
      if (LOG) printf("\n\t*** [USR] Client connection unexpectedly closed\n\n");
      quit = -1;
      break;
    }
    else
    {

      /* Ignore empty messages */
      if (LOG) printf("\n\t*** [USR] Empty messages are ignored\n\n");

    }

  } while (!quit);

  if (quit > 0) {

    /* User quit */
    if (LOG) printf("\n\t*** [USR] User quitting\n\n");

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
  if (LOG) fprintf(stderr, "\n\t*** [USR] User %s exiting with %d\n\n", user -> nickname, quit);
  terminate_receiver(user, waiting_queue);
  tlk_thread_join(&receiver_thread, (void **) &exit_code);

  /* Free resources and close thread */
  tlk_user_signout(user);
  tlk_thread_exit((tlk_exit_t) NULL);
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
    if (LOG) fprintf(stderr, "Cannot create new broker_thread\n");
    exit(EXIT_FAILURE);
  }

  /* Listen for incoming connections */
  if (LOG) fprintf(stderr, "Listen for incoming Connections\n\n");
  server_main_loop(port);

  /* Close Program */
  if (LOG) fprintf(stderr, "Close program\n\n");
  exit(EXIT_SUCCESS);
}
