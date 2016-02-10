#ifndef TLK_USERS_H
#define TLK_USERS_H

#include "tlk_linked_list.h"
#include "tlk_sockets.h"
#include "tlk_threads.h"
#include "tlk_common.h"
#include "tlk_semaphores.h"
#include "tlk_errors.h"

#include "string.h"

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
 * Returns 0 on success, propagates errors on fail
 */
int tlk_user_register (tlk_user_t *user);

/*
 * Delete user associated with @socket from extern users_list and deallocates memory, thread-safe
 * Returns 0 on success, propagates errors on fail
 */
int tlk_user_delete (tlk_user_t *user);

/*
 *
 *
 */
tlk_user_t *tlk_user_find(char *nickname);

#endif
