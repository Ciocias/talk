#include "../../include/tlk_server.h"

unsigned int incremental_id = 1;

/* Users handling data */
tlk_sem_t users_mutex;
unsigned int current_users;
tlk_user_t *users_list[MAX_USERS];

/* Input queue for broker thread */
tlk_queue_t *waiting_queue          = NULL;

/* Linked list of threads queues */
linked_list *threads_queues         = NULL;

typedef struct _queue_thread_arg_s {
  thread_node_t *node;
  tlk_socket_t socket;
} queue_thread_arg_t;

/*
 * Print server usage to standard error
 */
void usage_error_server (const char *prog_name) {
  fprintf(stderr, "Usage: %s <port_number>\n", prog_name);
  exit(EXIT_FAILURE);
}

/* Initialize server data */
unsigned short initialize_server (const char *argv[]) {

  int ret;
  unsigned short port = 0;

  if (LOG) printf("--> Parse port number\n");
  ret = parse_port_number(argv[1], &port);

  if (ret == -1) {
    fprintf(stderr, "Port not valid: must be between 1024 and 49151\n");

    exit(EXIT_FAILURE);
  }

  /* Initialize user data semaphore */
  if (LOG) printf("--> Initialize user data semaphore\n");
  ret = tlk_sem_init(&users_mutex, 1, 1);
  ERROR_HELPER(ret, "Cannot create users_mutex semaphore");

  /* Initialize global current_users */
  if (LOG) printf("--> Initialize global current_users\n");
  current_users = 0;

  /* Initialize global waiting_queue */
  if (LOG) printf("--> Initialize global waiting_queue\n");
  waiting_queue = tlk_queue_new(QUEUE_SIZE);

  if (waiting_queue == NULL) {
    fprintf(stderr, "Cannot create waiting queue with size %d\n", QUEUE_SIZE);
    exit(EXIT_FAILURE);
  }

  /* Initialize global threads_queues */
  if (LOG) printf("--> Initialize global thread_queues\n");
  threads_queues = linked_list_new();

  if (threads_queues == NULL) {
    fprintf(stderr, "Cannot create waiting queue with size %d\n", QUEUE_SIZE);
    exit(EXIT_FAILURE);
  }

  return port;
}

/* Server listening loop */
void server_main_loop (unsigned short port_number) {

  int ret;
  tlk_socket_t server_desc, client_desc;

  /* Set up socket and other connection data */
  if (LOG) printf("--> Set up socket and address data\n");
  struct sockaddr_in server_addr = { 0 };
  int sockaddr_len = sizeof(struct sockaddr_in);

  server_desc = tlk_socket_create(AF_INET, SOCK_STREAM, 0);
  ERROR_HELPER(server_desc, "Cannot create socket");

  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_family      = AF_INET;
  server_addr.sin_port        = port_number;

  int reuseaddr_opt = 1;

  if (LOG) printf("--> Set SO_REUSEADDR option\n");
  ret = setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
  ERROR_HELPER(ret, "Cannot set option SO_REUSEADDR");

  if (LOG) printf("--> Bind on socket\n");
  ret = tlk_socket_bind(server_desc, (struct sockaddr *) &server_addr, sockaddr_len);
  ERROR_HELPER(ret, "Cannot bind on socket");

  if (LOG) printf("--> Listen on socket\n");
  ret = tlk_socket_listen(server_desc, MAX_CONN_QUEUE);
  ERROR_HELPER(ret, "Cannot listen on socket");

  struct sockaddr_in *client_addr = (struct sockaddr_in *) calloc(1, sizeof(struct sockaddr_in));

  /* Wait for incoming connections */
  if (LOG) printf("--> Enter 'wait for incoming connections' loop\n");
  while (1)
  {
    client_desc = tlk_socket_accept(server_desc, (struct sockaddr *) client_addr, sockaddr_len);

    if (LOG) printf("--> Someone connected\n");

    if (client_desc == -1 && errno == TLK_EINTR) continue;
    ERROR_HELPER(client_desc, "Cannot accept on socket");

    /* Launch user handler thread */
    if (LOG) printf("--> Launch user handler thread\n");
    tlk_thread_t user_thread;

    tlk_user_t *new_user = (tlk_user_t *) malloc(sizeof(tlk_user_t));

    ret = tlk_sem_wait(&users_mutex);
    ERROR_HELPER(ret, "Cannot wait on users_mutex semaphore");

    new_user -> id        = incremental_id;
    new_user -> status    = IDLE;
    new_user -> socket    = client_desc;
    new_user -> address   = client_addr;
    new_user -> nickname  = (char *) malloc(NICKNAME_SIZE * sizeof(char));

    thread_node_t *node = (thread_node_t *) malloc(sizeof(thread_node_t));
    node -> id = incremental_id;

    incremental_id += 1;

    ret = tlk_sem_post(&users_mutex);
    ERROR_HELPER(ret, "Cannot post on users_mutex semaphore");

    node -> queue = tlk_queue_new(QUEUE_SIZE);
    if (node -> queue == NULL) {
      fprintf(stderr, "Cannot create waiting queue with size %d\n", QUEUE_SIZE);
      exit(EXIT_FAILURE);
    }

    ret = linked_list_add(threads_queues, (void *) node);
    if (ret == LINKED_LIST_NOK) {
      fprintf(stderr, "Cannot add new list item\n");
      exit(EXIT_FAILURE);
    }

    ret = tlk_thread_create(&user_thread, (tlk_thread_func) user_handler, (tlk_thread_args) new_user);
    PTHREAD_ERROR_HELPER(ret, "Cannot create thread");

    ret = tlk_thread_detach(&user_thread);
    PTHREAD_ERROR_HELPER(ret, "Cannot detach newly created thread");

    if (LOG) printf("--> Thread created, allocate new space for next client\n");
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
  if (LOG) printf("\n\t*** [BRK] Broker thread running\n\n");
  /* TODO: deallocate *msg  */
  tlk_message_t *msg = (tlk_message_t *) malloc(sizeof(tlk_message_t));
  while (1)
  {
    int ret;
    thread_node_t *node;

    /* Check for messages in the waiting queue */
    if (LOG) printf("\n\t*** [BRK] Check for messages in the waiting queue\n\n");
    ret = tlk_queue_dequeue(waiting_queue, msg);
    if (ret != 0) {
      fprintf(stderr, "Cannot read from waiting_queue\n");
      exit(EXIT_FAILURE);
    }

    /* Select correct thread queue from list */
    if (LOG) printf("\n\t*** [BRK] Select correct thread queue from list\n\n");

    linked_list_iterator *lli = linked_list_iterator_new(threads_queues);
    if (lli == NULL) {
      fprintf(stderr, "Cannot get thread specific queue\n");
      exit(EXIT_FAILURE);
    }

    while (lli != NULL) {

      node = NULL;
      node = (thread_node_t *) linked_list_iterator_getvalue(lli);
      if (node == NULL) {
        fprintf(stderr, "Cannot get thread specific queue\n");
        exit(EXIT_FAILURE);
      }

      if (node -> id == (msg -> receiver) -> id) break;

      lli = linked_list_iterator_next(lli);
    }

    if (node == NULL) continue;

    /* Sort message in correct queue */
    if (LOG) printf("\n\t*** [BRK] Sort message in correct queue\n\n");
    ret = tlk_queue_enqueue(node -> queue, (const tlk_message_t *) msg);
    if (ret != 0) {
      fprintf(stderr, "Cannot enqueue message\n");
      exit(EXIT_FAILURE);
    }

  }
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
  thread_node_t *t_node;
  tlk_user_t *user = (tlk_user_t *) arg;

  if (LOG) printf("\n\t*** [USR] User handler thread running\n\n");

  linked_list_iterator *lli = linked_list_iterator_new(threads_queues);
  if (lli == NULL) {
    fprintf(stderr, "Cannot get thread specific queue\n");
    close_and_free_session(user);
  }

  while (lli != NULL) {

    t_node = NULL;
    t_node = (thread_node_t *) linked_list_iterator_getvalue(lli);
    if (t_node == NULL) {
      fprintf(stderr, "Cannot get thread specific queue\n");
      close_and_free_session(user);
    }

    if (t_node -> id == user -> id) break;

    lli = linked_list_iterator_next(lli);
  }

  /* Wait for a join message from client */
  if (LOG) printf("\n\t*** [USR] Wait for a join message from client\n\n");
  int join_msg_len = recv_msg(user -> socket, msg, MSG_SIZE);

  if (join_msg_len < 0) {
    tlk_thread_exit((tlk_exit_t) NULL);
  } else if (parse_join_msg(msg, strlen(msg), user -> nickname) != 0) {

    snprintf(msg, strlen(JOIN_FAILED) + 1, "%s", JOIN_FAILED);

    if (LOG) printf("\n\t*** [USR] Failed to join new user as %s\n\n", user -> nickname);
    send_msg(user -> socket, msg);

    tlk_thread_exit((tlk_exit_t) NULL);
  }

  /* Register new user with given nickname */
  if (LOG) printf("\n\t*** [USR] Register new user as %s\n\n", user -> nickname);
  ret = tlk_user_register(user);
  if (ret != 0) {

    snprintf(msg, strlen(REGISTER_FAILED) + 1, "%s", REGISTER_FAILED);

    if (LOG) printf("\n\t*** [USR] Failed to register new user as %s\n\n", user -> nickname);
    send_msg(user -> socket, msg);

    tlk_thread_exit((tlk_exit_t) NULL);
  }

  /* If everything runs fine notify the client */
  snprintf(msg, strlen(JOIN_SUCCESS) + 1, "%s", JOIN_SUCCESS);

  if (LOG) printf("\n\t*** [USR] Successfully registered new user %s!\n\n", user -> nickname);
  ret = send_msg(user -> socket, msg);

  if (ret == TLK_SOCKET_ERROR) {
    if (LOG) printf("\n\t*** [USR] Cannot notify the client: delete new user and exit...\n\n");

    tlk_user_delete(user);
    tlk_thread_exit((tlk_exit_t) NULL);
  }

  /* Send welcome & help (includes commands list) */
  if (LOG) printf("\n\t*** [USR] Sending welcome and help\n\n");

  snprintf(msg, strlen(WELCOME_MSG) + strlen(user -> nickname), WELCOME_MSG, user -> nickname);

  send_msg(user -> socket, msg);
  send_help(user -> socket);

  if (LOG) printf("\n\t*** [USR] User chat session started\n\n");
  user_chat_session(user, t_node);

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
  queue_thread_arg_t *args = (queue_thread_arg_t *) arg;

  while (1)
  {
    /* Check own thread queue (reading) and send to user */
    if (LOG) printf("\n\t*** [QCR] Check own thread queue\n\n");
    ret = tlk_queue_dequeue((args -> node) -> queue, tlk_msg);
    if (ret) {
      if (LOG) printf("\n\t*** [QCR] Dequeuing error, exiting...\n\n");
      tlk_thread_exit((tlk_exit_t) NULL);
    }

    if (tlk_msg != NULL) {
      /* TODO: quit command handling */

      if (strncmp(tlk_msg -> content, DIE_MSG, strlen(DIE_MSG)) == 0) {
				printf("\n\t*** [QCR] Exiting...\n\n");
				tlk_thread_exit((tlk_exit_t) NULL);
      }

      /* Not a command: send it to our user */
      ret = send_msg(args -> socket, tlk_msg -> content);
      if (ret == TLK_SOCKET_ERROR) {
        if (LOG) printf("\n\t*** [QCR] Cannot send message to user: exiting...\n\n");
        tlk_thread_exit((tlk_exit_t) NULL);
      }
    }
  }
}

/* TODO: commands_handler description */
void commands_handler(int *quit, tlk_user_t *user, tlk_queue_t *queue, char msg[MSG_SIZE]) {

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
    send_list(user -> socket, users_list);

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
      return;
    }

    /* Try to find user with given nickname */
    user -> listener = tlk_user_find(msg + talk_command_len + 1);
    if (user -> listener == NULL) {
      if (LOG) printf("\n\t*** [USR] Unable to start new talk session \n\n");

      snprintf(error_msg, strlen(USER_NOT_FOUND), USER_NOT_FOUND);

      ret = send_msg(user -> socket, error_msg);
      if (ret == TLK_SOCKET_ERROR) {
        if (LOG) printf("\n\t*** [USR] Cannot send error_msg to user %s\n\n", user -> nickname);

        terminate_receiver(user, queue);
        close_and_free_session(user);
      }

    }

    /* Try to start a talking session with the given nickname */
    ret = talk_session(user, queue);
    if (ret < 0) {
      if (LOG) printf("\n\t*** [USR] Error enqueuing in waiting queue, exiting...\n\n");

      terminate_receiver(user, queue);
      close_and_free_session(user);
    }
  }
  else if (strncmp(msg + 1, QUIT_COMMAND, strlen(QUIT_COMMAND)) == 0)
  {

    if (LOG) printf("\n\t*** [USR] User asked to quit\n\n");
    if (user -> status == TALKING) return;

    *quit = 1;

    ret = terminate_receiver(user, queue);
    if (ret) {
      if (LOG) printf("\n\t*** [USR] Error enqueuing in waiting queue, exiting...\n\n");
      tlk_thread_exit((tlk_exit_t) NULL);
    }

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
        if (LOG) printf("\n\t*** [USR] Error enqueuing in waiting queue, exiting...\n\n");
        tlk_thread_exit((tlk_exit_t) NULL);
      }

      /* Unset both listeners */
      (user -> listener) -> listener = NULL;
      user -> listener = NULL;
    }
    else
    {
      sprintf(error_msg, "Your are idle");
      ret = send_msg(user -> socket, error_msg);
      if (ret == TLK_SOCKET_ERROR) {
        if (LOG) printf("\n\t*** [USR] Cannot send message to user: exiting...\n\n");
      }
    } /* (user -> status == TALKING) */
  }
  else
  {

    if (LOG) printf("\n\t*** [USR] User didn't ask\n\n");
    ret = send_unknown(user -> socket);
    if (ret == TLK_SOCKET_ERROR) {
      if (LOG) printf("\n\t*** [USR] Cannot send error_msg to user %s\n\n", user -> nickname);
      tlk_thread_exit((tlk_exit_t) NULL);
    }

  } /* Unknown command */

}

/* Chat session handler */
void user_chat_session (tlk_user_t *user, thread_node_t *t_node) {
  int ret, exit_code;
  int quit = 0;

  char msg[MSG_SIZE];

  /* Launch user_receiver thread */
  tlk_thread_t receiver_thread;
	queue_thread_arg_t *args = (queue_thread_arg_t *)malloc(sizeof(queue_thread_arg_t));
  if (LOG) printf("\n-> Launch %s queue-checking routine thread\n\n", user->nickname);

	args->node = t_node;
	args->socket = user->socket;

	ret = tlk_thread_create(&receiver_thread, (tlk_thread_func)user_receiver, (tlk_thread_args)args);
	PTHREAD_ERROR_HELPER(ret, "Cannot create user_receiver thread");


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
        commands_handler(&quit, user, waiting_queue, msg);

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
              if (LOG) printf("\n\t*** [USR] Error enqueuing in waiting queue, exiting...\n\n");
              tlk_thread_exit((tlk_exit_t) NULL);
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

      ret = terminate_receiver(user, waiting_queue);
      if (ret) {
        if (LOG) printf("\n\t*** [USR] Error enqueuing in waiting queue, exiting...\n\n");
        tlk_thread_exit((tlk_exit_t) NULL);
      }

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

  /* Wait for user_receiver termination */
  ret = tlk_thread_join(&receiver_thread, &exit_code);
  PTHREAD_ERROR_HELPER(ret, "Cannot join receiver_thread thread");

  /* Deallocate thread_node_t struct */
  tlk_queue_free(t_node -> queue);
  linked_list_remove(threads_queues, (void *) t_node);

  ret = tlk_user_delete(user);
  tlk_thread_exit((tlk_exit_t) NULL);
}

int main (int argc, const char *argv[]) {

  int ret;

  /* Initialize server data */
  if (LOG) printf("\n-> Initialize server data\n\n");
  if (argc != 2) {
    usage_error_server(argv[0]);
  }
  unsigned short port = initialize_server(argv);

  /* Launch broker thread */
  if (LOG) printf("\n-> Launch broker thread\n\n");
  tlk_thread_t broker_thread;

  ret = tlk_thread_create(&broker_thread, (tlk_thread_func) broker_routine,  NULL);
  PTHREAD_ERROR_HELPER(ret, "Cannot create thread");

  ret = tlk_thread_detach(&broker_thread);
  PTHREAD_ERROR_HELPER(ret, "Cannot detach newly created thread");

  /* Listen for incoming connections */
  if (LOG) printf("\n-> Listen for incoming Connections\n\n");
  server_main_loop(port);

  /* Close Program */
  if (LOG) printf("\n-> Close program\n\n");
  exit(EXIT_SUCCESS);
}
