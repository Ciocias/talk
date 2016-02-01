#include "../../include/tlk_server.h"

unsigned int incremental_id = 1;

/* Users handling data */
tlk_sem_t users_mutex;
unsigned int current_users;
tlk_user_t *users_list[MAX_USERS];

tlk_queue_t *waiting_queue          = NULL; /* Input queue for broker thread */
linked_list *threads_queues         = NULL; /* Linked list of threads queues */

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
  ret = tlk_sem_init(&users_mutex, 1);
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

  struct sockaddr_in *client_addr = calloc(1, sizeof(struct sockaddr_in));

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

    ret = tlk_thread_detach(user_thread);
    PTHREAD_ERROR_HELPER(ret, "Cannot detach newly created thread");

    if (LOG) printf("--> Thread created, allocate new space for next client\n");
    client_addr = calloc(1, sizeof(struct sockaddr_in));
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
  if (LOG) printf("\n\t*** [USR] User handler thread running\n\n");
  int ret;
  char msg[MSG_SIZE];
  thread_node_t *t_node;
  tlk_user_t *user = (tlk_user_t *) arg;

  linked_list_iterator *lli = linked_list_iterator_new(threads_queues);
  if (lli == NULL) {
    fprintf(stderr, "Cannot get thread specific queue\n");
    close_and_free_chat_session(user);
  }

  while (lli != NULL) {

    t_node = NULL;
    t_node = (thread_node_t *) linked_list_iterator_getvalue(lli);
    if (t_node == NULL) {
      fprintf(stderr, "Cannot get thread specific queue\n");
      close_and_free_chat_session(user);
    }

    if (t_node -> id == user -> id) break;

    lli = linked_list_iterator_next(lli);
  }

  /* Wait for a join message from client */
  if (LOG) printf("\n\t*** [USR] Wait for a join message from client\n\n");
  int join_msg_len = recv_msg(user -> socket, msg, MSG_SIZE);

  if (join_msg_len < 0) {
    tlk_thread_exit(NULL);
  } else if (parse_join_msg(msg, strlen(msg), user -> nickname) != 0) {

    snprintf(msg, strlen(JOIN_FAILED) + 1, "%s", JOIN_FAILED);

    if (LOG) printf("\n\t*** [USR] Failed to join new user as %s\n\n", user -> nickname);
    send_msg(user -> socket, msg);

    tlk_thread_exit(NULL);
  }

  /* Register new user with given nickname */
  if (LOG) printf("\n\t*** [USR] Register new user as %s\n\n", user -> nickname);
  ret = tlk_user_register(user);
  if (ret != 0) {

    snprintf(msg, strlen(REGISTER_FAILED) + 1, "%s", REGISTER_FAILED);

    if (LOG) printf("\n\t*** [USR] Failed to register new user as %s\n\n", user -> nickname);
    send_msg(user -> socket, msg);

    tlk_thread_exit(NULL);
  }

  /* If everything runs fine notify the client */
  snprintf(msg, strlen(JOIN_SUCCESS) + 1, "%s", JOIN_SUCCESS);

  if (LOG) printf("\n\t*** [USR] Successfully registered new user %s!\n\n", user -> nickname);
  send_msg(user -> socket, msg);

  /* Send welcome & help (includes commands list) */
  if (LOG) printf("\n\t*** [USR] Sending welcome and help\n\n");

  snprintf(msg, strlen(WELCOME_MSG) + strlen(user -> nickname), WELCOME_MSG, user -> nickname);

  send_msg(user -> socket, msg);
  send_help(user -> socket);

  /* User chat session */
  if (LOG) printf("\n\t*** [USR] User chat session started\n\n");
  user_chat_session(t_node, user);

  /* We do a clean exit inside close_and_free_chat_session() */
  return NULL;
}

/* Chat session handler */
void user_chat_session (thread_node_t *t_node, tlk_user_t *user) {
  int ret;
  int quit = 0;
  tlk_user_t *listener;
  tlk_message_t *tlk_msg;

  char msg[MSG_SIZE];
  char error_msg[MSG_SIZE];

  do {
    printf("\n\n%s is %s\n\n", user -> nickname, (user -> status == IDLE ? "IDLE":"TALKING"));

    /* Receive data from user */
    if (LOG) printf("\n\t*** [USR] Receive data from user\n\n");
    int len = recv_msg(user -> socket, msg, MSG_SIZE);

    if (len > 0) {

      /* Handle server commands */
      if (msg[0] == COMMAND_CHAR) {
        if (LOG) printf("\n\t*** [USR] Handle server commands recognition and execution\n\n");

        if (strcmp(msg + 1, HELP_COMMAND) == 0) {

          if (LOG) printf("\n\t*** [USR] User asked for help\n\n");
          send_help(user -> socket);

        } else if (strcmp(msg + 1, LIST_COMMAND) == 0) {

          if (LOG) printf("\n\t*** [USR] User asked the list\n\n");
          send_users_list(user -> socket, users_list);

        } else if (strncmp(msg + 1, TALK_COMMAND, strlen(TALK_COMMAND)) == 0) {

          if (LOG) printf("\n\t*** [USR] User asked to talk\n\n");
          /* TODO: implement talk command */

          /* Try to start a talking session with the given nickname */

          int ret;
          char *nickname = malloc(NICKNAME_SIZE * sizeof(char));
          size_t msg_len = strlen(msg);

          ret = strncmp(msg, "/talk", strlen("/talk"));

          if (msg_len > strlen("/talk") && ret == 0) {

            snprintf(nickname, msg, "%s", msg + strlen("/talk") + 1);

          }

          listener = tlk_user_find(nickname);
          if (listener == NULL) {

            if (LOG) printf("\n\t*** [USR] Unable to find user %s\n\n", nickname);
            snprintf(error_msg, "Unable to find user %s", nickname, strlen(nickname));
            send_msg(user -> socket, error_msg);

            continue;
          } else {
            if (listener -> status == IDLE) {

              /* Start talking with listener */
              user -> status = TALKING;
              listener -> status = TALKING;
            }
            printf("\n\n%s is %s\n\n", user -> nickname, (user -> status == IDLE ? "IDLE":"TALKING"));
            printf("\n\n%s is %s\n\n", listener -> nickname, (listener -> status == IDLE ? "IDLE":"TALKING"));
          }
        } else if (strcmp(msg + 1, QUIT_COMMAND) == 0) {

          if (LOG) printf("\n\t*** [USR] User asked to quit\n\n");
          quit = 1;

        } else {

          if (LOG) printf("\n\t*** [USR] User didn't ask\n\n");
          sprintf(error_msg, "Unknown command, type %c%s to get a list of available commands", COMMAND_CHAR, HELP_COMMAND);
          send_msg(user -> socket, error_msg);

        }

      } else {

        /* TODO: implement talk */
        /*
         *  TODO: UPDATE:
         *  Status system seems to work (not so good for now); some bugs may lurk beneath 
         */

        /* Are we talking ? */
        if (user -> status == TALKING && listener != NULL) {

          /* Pack message to send */
          if (LOG) printf("\n\t*** [USR] Set up message struct\n\n");
          tlk_msg = (tlk_message_t *) malloc(sizeof(tlk_message_t));

          tlk_msg -> id         = listener -> id;
          tlk_msg -> sender     = user;
          tlk_msg -> receiver   = listener;

          tlk_msg -> content    = (char *) calloc(MSG_SIZE, sizeof(char));

          sprintf(tlk_msg -> content, msg);

          /* Send packed message to listener using the queue system */
          if (LOG) printf("\n\t*** [USR] Enqueue in waiting queue\n\n");
          ret = tlk_queue_enqueue(waiting_queue, tlk_msg);
          ERROR_HELPER(ret, "Cannot enqueue new message");

          /* Check own thread queue (reading) and send to user */
          if (LOG) printf("\n\t*** [USR] Check own thread queue\n\n");
          ret = tlk_queue_dequeue(t_node -> queue, tlk_msg);
          ERROR_HELPER(ret, "Cannot dequeue message");

          send_msg(user -> socket, tlk_msg -> content);
        }
      }


    }

    if (len < 0) {
      /* Client connection unexpectedly closed */
      if (LOG) printf("\n\t*** [USR] Client connection unexpectedly closed\n\n");
      quit = -1;
    } else {
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

  ret = tlk_user_delete(user -> socket);
  tlk_thread_exit(NULL);
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

  ret = tlk_thread_detach(broker_thread);
  PTHREAD_ERROR_HELPER(ret, "Cannot detach newly created thread");

  /* Listen for incoming connections */
  if (LOG) printf("\n-> Listen for incoming Connections\n\n");
  server_main_loop(port);

  /* Close Program */
  if (LOG) printf("\n-> Close program\n\n");
  exit(EXIT_SUCCESS);
}
