#include "../../include/tlk_semaphores.h"

/*
 * Initialize the semaphore pointed by @sem with @value
 * Returns 0 on success, -1 on failure
 */
int tlk_sem_init(tlk_sem_t *sem, unsigned int value, unsigned int max_value) {

  int ret = 0;

#if defined(_WIN32) && _WIN32

	*sem = CreateSemaphore(NULL, value, max_value, NULL);

  ret = (*sem != NULL? 0 : -1);

#elif defined(__linux__) && __linux__

  ret = sem_init(sem, 0, value);

#endif

  return ret;
}

/*
 * Close and destroy @sem
 * Returns 0 on success, -1 on failure
 */
int tlk_sem_destroy(tlk_sem_t *sem) {

  int ret = 0;

#if defined(_WIN32) && _WIN32

  ret = (CloseHandle(sem) != FALSE ? 0 : -1);

#elif defined(__linux__) && __linux__

  ret = sem_destroy(sem);

#endif

  return ret;
}

/*
 * Perform a wait operation on @sem
 * Returns 0 on success, -1 on failure
 */
int tlk_sem_wait(tlk_sem_t *sem) {

  int ret = 0;

#if defined(_WIN32) && _WIN32

	ret = WaitForSingleObject(*sem, INFINITE)/* != WAIT_FAILED ? 0 : -1)*/;

	switch (ret) {
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			printf("\nwait_timeout\n");
			break;
		case WAIT_FAILED:
			printf("\nwait_failed\n");
			printf("sem is null: %d\nGetLastError: %d\n", *sem == NULL, GetLastError());
			break;
	}
#elif defined(__linux__) && __linux__

  ret = sem_wait(sem);

#endif

  return ret;

}

/*
 * Perform a post operation on @sem
 * Returns 0 on success, -1 on failure
 */
int tlk_sem_post(tlk_sem_t *sem) {

  int ret = 0;

#if defined(_WIN32) && _WIN32

  ret = (ReleaseSemaphore(*sem, 1, NULL) != FALSE ? 0 : -1);

#elif defined(__linux__) && __linux__

  ret = sem_post(sem);

#endif

  return ret;

}
