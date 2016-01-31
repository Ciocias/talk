#include "../../include/tlk_users.h"

extern tlk_sem_t users_mutex;
extern tlk_user_t *users_list[];
extern unsigned int current_users;

/*
 * Register @user into extern users_list if possible, thread-safe
 * Returns 0 on success, propagates errors on fail
 */
int tlk_user_register (tlk_user_t *user) {

  int ret = 0;

  ret = tlk_sem_wait(&users_mutex);
  ERROR_HELPER(ret, "Cannot wait on users_mutex semaphore");

  /* Check for max users limit */
  if (current_users == MAX_USERS) {
    ret = tlk_sem_post(&users_mutex);
    ERROR_HELPER(ret, "Cannot post on users_mutex semaphore");

    return 1;
  }

  /* Check if given nickname is already taken */
  int i;
  for (i = 0; i < current_users; i++) {

    if ( strcmp(user -> nickname, users_list[i] -> nickname) == 0 ) {
      ret = tlk_sem_post(&users_mutex);
      ERROR_HELPER(ret, "Cannot post on users_mutex semaphore");

      return 1;
    }
  }

  /* Insert new user in users_list */
  users_list[current_users] = user;
  current_users++;

  /* Notify new user presence to all users */
  /* TODO: implement broadcast messages (server) */

  ret = tlk_sem_post(&users_mutex);
  ERROR_HELPER(ret, "Cannot post on users_mutex semaphore");

  return ret;
}

/*
 * Delete user associated with @socket from extern users_list and deallocates memory, thread-safe
 * Returns 0 on success, propagates errors on fail
 */
int tlk_user_delete (tlk_socket_t socket) {

  int ret;

  ret = tlk_sem_wait(&users_mutex);
  ERROR_HELPER(ret, "Cannot wait on users_mutex semaphore");

  int i;
  for (i = 0; i < current_users; i++) {
    if (users_list[i] -> socket == socket) {
      /* TODO: notify all users */

      ret = tlk_socket_close(socket);
      ERROR_HELPER(ret, "Cannot close user socket");

      free(users_list[i] -> address);
      free(users_list[i]);

      for (; i < current_users - 1; i++) {
        users_list[i] = users_list[i + 1]; /* Shift all elements by 1 */
        users_list[i] -> id = i;
      }
      current_users--;

      ret = tlk_sem_post(&users_mutex);
      ERROR_HELPER(ret, "Cannot post on users_mutex semaphore");

      return ret;
    }
  }

  ret = tlk_sem_post(&users_mutex);
  ERROR_HELPER(ret, "Cannot post on users_mutex semaphore");

  return ret;
}
