#include "../../include/users.h"

#include <stdlib.h>
#include <string.h>

extern tlk_sem_t users_mutex;
extern tlk_user_t *users_list[];
extern unsigned int current_users;

/*
 * Creates and returns a new user structure
 * returns a pointer to the user on success, NULL on failure
 */
tlk_user_t *tlk_user_new (unsigned int incremental_id, tlk_socket_t client_desc, struct sockaddr_in *client_addr) {

  tlk_user_t *new_user = (tlk_user_t *) malloc(sizeof(tlk_user_t));

  new_user -> id        = incremental_id;
  new_user -> status    = IDLE;

  new_user -> socket    = client_desc;
  new_user -> address   = client_addr;

  new_user -> nickname  = (char *) malloc(NICKNAME_SIZE * sizeof(char));

  new_user -> queue     = tlk_queue_new(QUEUE_SIZE);
  if (new_user -> queue == NULL) return NULL;

  return new_user;
}

/*
 * Deallocates user structure
 * Returns:
 *   0 on success,
 *   -1 on queue errors,
 *   TLK_SOCKET_ERROR on socket failure
 */
int tlk_user_free (tlk_user_t *user) {

  int ret = 0;

  ret = tlk_socket_close(user -> socket);
  if (ret == TLK_SOCKET_ERROR) return ret;

  ret = tlk_queue_free(user -> queue);
  if (ret) return ret;

  free(user -> nickname);
  free(user -> address);
  free(user);

  return ret;
}

/*
 * Register @user into extern users_list if possible, thread-safe
 * Returns:
 *   0 on success,
 *   -1 on semaphore errors,
 *   MAX_USERS_ERROR if users count exceed the limit,
 *   NICKNAME_ERROR if nickname is already taken;
 */
int tlk_user_signin (tlk_user_t *user) {

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
  ++current_users;

  /* Done, release the semaphore and exit */
  ret = tlk_sem_post(&users_mutex);
  return ret;
}

/*
 * Delete @user from extern users_list and frees memory, thread-safe
 * Returns:
 *   0 on success,
 *   -1 on semaphore errors,
 *   -2 on queue errors,
 *   TLK_SOCKET_ERROR on socket errors
 */
int tlk_user_signout (tlk_user_t *user) {

  int ret = 0;

  ret = tlk_sem_wait(&users_mutex);
  if (ret) return ret;

  unsigned int i;
  for (i = 0; i < current_users; i++) {
    if (users_list[i] -> id == user -> id) {

    ret = tlk_user_free(user);
	  if (ret) {
		  if (ret == TLK_SOCKET_ERROR) return TLK_SOCKET_ERROR;
		  return -2;
	  }
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
 * Search for a registered user called @nickname
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
