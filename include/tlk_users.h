#ifndef TLK_USERS_H
#define TLK_USERS_H

#include "tlk_sockets.h"
#include "tlk_threads.h"
#include "tlk_semaphores.h"
#include "tlk_common.h"

#define MAX_USERS_ERROR 1
#define NICKNAME_ERROR  2

typedef enum {
  IDLE,
  TALKING
} user_status;

typedef struct _tlk_user_s {
  int id;
  user_status status;
  struct _tlk_user_s *listener;
  tlk_socket_t socket;
  struct sockaddr_in *address;
  char *nickname;
} tlk_user_t;

typedef struct _tlk_message_s {
  int id;
  tlk_user_t *sender;
  tlk_user_t *receiver;
  char *content;
} tlk_message_t;

/*
 * Register @user into extern users_list if possible, thread-safe
 * Returns:
 *   0 on success,
 *   -1 on semaphore errors,
 *   MAX_USERS_ERROR if users count exceed the limit,
 *   NICKNAME_ERROR if nickname is already taken;
 */
int tlk_user_register (tlk_user_t *user);

/*
 * Delete user associated with @socket from extern users_list and deallocates memory, thread-safe
 * Returns:
 *   0 on success,
 *   -1 on semaphore errors,
 *   TLK_SOCKET_ERROR on socket errors
 */
int tlk_user_delete (tlk_user_t *user);

/*
 * Try to find @nickname as a registered user
 * Returns a pointer if found, NULL on failure
 */
tlk_user_t *tlk_user_find(char *nickname);

#endif /* TLK_USERS_H */
