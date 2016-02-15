#include "../../include/tlk_users.h"

#include <stdlib.h>
#include <string.h>

extern tlk_sem_t users_mutex;
extern tlk_user_t *users_list[];
extern unsigned int current_users;

/*
 * Register @user into extern users_list if possible, thread-safe
 * Returns:
 *   0 on success,
 *   -1 on semaphore errors,
 *   MAX_USERS_ERROR if users count exceed the limit,
 *   NICKNAME_ERROR if nickname is already taken;
 */
int tlk_user_register (tlk_user_t *user) {

  int ret = 0;

  ret = tlk_sem_wait(&users_mutex);
  if (ret) return ret;

  /* Check for max users limit */
  if (current_users >= MAX_USERS) {
    ret = tlk_sem_post(&users_mutex);
    if (ret) return ret;

    return MAX_USERS_ERROR;
  }

  /* Check if given nickname is already taken */
  unsigned int i;
  for (i = 0; i < current_users; i++) {

    if ( strncmp(user -> nickname, users_list[i] -> nickname, strlen(user -> nickname)) == 0 ) {
      ret = tlk_sem_post(&users_mutex);
      if (ret) return ret;

      return NICKNAME_ERROR;
    }
  }

  /* Insert new user in users_list */
  users_list[current_users] = user;
  current_users++;

  /* Done, release the semaphore and exit */
  ret = tlk_sem_post(&users_mutex);
  return ret;
}

/*
 * Delete user associated with @socket from extern users_list and frees memory, thread-safe
 * Returns:
 *   0 on success,
 *   -1 on semaphore errors,
 *   TLK_SOCKET_ERROR on socket errors
 */
int tlk_user_delete (tlk_user_t *user) {

  int ret = 0;

  ret = tlk_sem_wait(&users_mutex);
  if (ret) return ret;

  unsigned int i;
  for (i = 0; i < current_users; i++) {
    if (users_list[i] -> id == user -> id) {

      ret = tlk_socket_shutdown(user -> socket, TLK_SOCKET_RW);
      if (ret == TLK_SOCKET_ERROR) return ret;

      ret = tlk_socket_close(user -> socket);
      if (ret == TLK_SOCKET_ERROR) return ret;

      free(users_list[i] -> nickname);
      free(users_list[i] -> address);
      free(users_list[i]);

      users_list[i] = NULL;

      for (; i < current_users - 1; i++) {
        users_list[i] = users_list[i + 1]; /* Shift all elements by 1 */
      }

      users_list[i] = NULL;
      current_users--;

      ret = tlk_sem_post(&users_mutex);
      return ret;
    }
  }

  ret = tlk_sem_post(&users_mutex);
  return ret;
}

/*
 * Try to find @nickname as a registered user
 * Returns a pointer if found, NULL on failure
 */
tlk_user_t *tlk_user_find(char *nickname) {

  int ret;
  tlk_user_t *result = NULL;

  ret = tlk_sem_wait(&users_mutex);
  if (ret) return NULL;

  unsigned int i;
  size_t nickname_len = strlen(nickname);

  for (i = 0; i < current_users; i++) {
    if (strncmp(nickname, users_list[i] -> nickname, nickname_len) == 0) {
      result = users_list[i];
      break;
    }
  }

  ret = tlk_sem_post(&users_mutex);
  if (ret) return NULL;

  return result;
}
