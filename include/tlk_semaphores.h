#ifndef TLK_SEMAPHORES_H
#define TLK_SEMAPHORES_H

#if defined(_WIN32) && _WIN32
#define _WINSOCKAPI_

#include <Windows.h>

typedef HANDLE tlk_sem_t;

#elif defined(__linux__) && __linux__

#include <semaphore.h>

typedef sem_t tlk_sem_t;

#endif

/*
 * Initialize the semaphore pointed by @sem with @value
 * @max_value is needed on windows
 * Returns 0 on success, -1 on failure
 */
int tlk_sem_init (tlk_sem_t *sem, unsigned int value, unsigned int max_value);

/*
 * Close and destroy @sem
 * Returns 0 on success, -1 on failure
 */
int tlk_sem_destroy (tlk_sem_t *sem);

/*
 * Perform a wait operation on @sem
 * Returns 0 on success, -1 on failure
 */
int tlk_sem_wait (tlk_sem_t *sem);

/*
 * Perform a post operation on @sem
 * Returns 0 on success, -1 on failure
 */
int tlk_sem_post (tlk_sem_t *sem);

#endif /* TLK_SEMAPHORES_H */
