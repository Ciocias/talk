#ifndef TLK_SERVER_H
#define TLK_SERVER_H

#include "tlk_sockets.h"
#include "tlk_threads.h"
#include "tlk_semaphores.h"
#include "users.h"
#include "msg_queue.h"
#include "util.h"
#include "common.h"
/* Initialize server data */
unsigned short initialize_server (const char *argv[]);

/* Listen for incoming connections */
void server_main_loop (unsigned short port_number);

/* Broker thread */
#if defined(_WIN32) && _WIN32
DWORD WINAPI broker_routine (LPVOID arg);
#elif defined(__linux__) && __linux__
void * broker_routine (void);
#endif

/* User handler thread */
#if defined(_WIN32) && _WIN32
DWORD WINAPI user_handler (LPVOID arg);
#elif defined(__linux__) && __linux__
void * user_handler (void *arg);
#endif

/* User queue-checking thread */
#if defined(_WIN32) && _WIN32
DWORD WINAPI user_receiver (LPVOID arg);
#elif defined(__linux__) && __linux__
void * user_receiver (void *arg);
#endif

/* TODO: commands_handler description */
int commands_handler (tlk_user_t *user, char msg[MSG_SIZE]);

/* TODO: message_handler description */
int message_handler (tlk_user_t *user, char msg[MSG_SIZE]);

/* Chat session handler */
void user_chat_session (tlk_user_t *user);

#endif
