#ifndef TLK_CLIENT_H
#define TLK_CLIENT_H

#include "tlk_sockets.h"
#include "tlk_threads.h"
#include "tlk_util.h"
#include "tlk_common.h"

/* Initialize client data */
tlk_socket_t initialize_client (const char *argv[]);

/* Send JOIN command */
void join_server(tlk_socket_t *socket, const char *nickname);

/* Handle chat session */
void chat_session (tlk_socket_t socket, const char *nickname);

/* Receiver thread */
#if defined(_WIN32) && _WIN32
DWORD WINAPI receiver (LPVOID arg);
#elif defined(__linux__) && __linux__
void * receiver (void *arg);
#endif

/* Sender thread */
#if defined(_WIN32) && _WIN32
DWORD WINAPI sender (LPVOID arg);
#elif defined(__linux__) && __linux__
void * sender (void *arg);
#endif

#endif
