#include "../../include/tlk_semaphores.h"

/*
 * Initialize the semaphore pointed by @sem with @value
 * Returns 0 on success, propagates errors on fail
 */
int tlk_sem_init(tlk_sem_t *sem, unsigned int value) {

  int ret;

#if defined(_WIN32) && _WIN32

  sem = CreateSemaphore(NULL, value, value, NULL);

  ret = (sem != NULL ? 0 : 1);

#elif defined(__linux__) && __linux__

  ret = sem_init(sem, 0, value);

#endif

  return ret;
}

/*
 * Close and destroy @sem
 * Returns 0 on success, propagates errors on fail
 */
int tlk_sem_destroy(tlk_sem_t *sem) {

  int ret;

#if defined(_WIN32) && _WIN32

  ret = (CloseHandle(sem) != FALSE ? 0 : 1);

#elif defined(__linux__) && __linux__

  ret = sem_destroy(sem);

#endif

  return ret;
}

/*
 * Perform a wait operation on @sem
 * Returns 0 on success, propagates errors on fail
 */
int tlk_sem_wait(tlk_sem_t *sem) {

  int ret;

#if defined(_WIN32) && _WIN32

  ret = (WaitForSingleObject(sem, 0L) != WAIT_FAILED ? 0 : 1);

#elif defined(__linux__) && __linux__

  ret = sem_wait(sem);

#endif

  return ret;

}

/*
 * Perform a post operation on @sem
 * Returns 0 on success, propagates errors on fail
 */
int tlk_sem_post(tlk_sem_t *sem) {

  int ret;

#if defined(_WIN32) && _WIN32

  ret = (ReleaseSemaphore(sem, 1, NULL) != FALSE ? 0 : 1);

#elif defined(__linux__) && __linux__

  ret = sem_post(sem);

#endif

  return ret;

}
