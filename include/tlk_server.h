#ifndef TLK_SERVER_H
#define TLK_SERVER_H

#include <string.h>

#include "tlk_common.h"
#include "tlk_util.h"
#include "tlk_users.h"
#include "tlk_msg_queue.h"
#include "tlk_errors.h"
#include "tlk_sockets.h"
#include "tlk_semaphores.h"
#include "tlk_linked_list.h"
#include "tlk_threads.h"

/* Thread 'user_handler' args struct */
typedef struct _thread_node_s {
  int id;
  tlk_queue_t *queue;
} thread_node_t;

/* Initialize server data */
unsigned short initialize_server (const char *argv[]);

/* Listen for incoming connections */
void server_main_loop (unsigned short port_number);

/* Broker thread */
#if defined(_WIN32) && _WIN32
DWORD WINAPI broker_routine (LPVOID arg);
#elif defined(__linux__) && __linux__
void * broker_routine (void *arg);
#endif

/* User handler thread */
#if defined(_WIN32) && _WIN32
DWORD WINAPI user_handler (LPVOID arg);
#elif defined(__linux__) && __linux__
void * user_handler (void *arg);
#endif

/* User queue-checking thread */
#if defined(_WIN32) && _WIN32
DWORD WINAPI queue_checker_routine (LPVOID arg);
#elif defined(__linux__) && __linux__
void * queue_checker_routine (void *arg);
#endif

/* Chat session handler */
void user_chat_session (tlk_user_t *user);

#endif
