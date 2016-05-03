#ifndef TLK_THREADS_H
#define TLK_THREADS_H

#if defined(_WIN32) && _WIN32
#define _WINSOCKAPI_

#include <Windows.h>

#define TLK_THREAD_INVALID            INVALID_HANDLE_VALUE

typedef HANDLE                        tlk_thread_t;
typedef LPVOID                        tlk_thread_args;
typedef DWORD                         tlk_exit_t;

typedef LPTHREAD_START_ROUTINE        tlk_thread_func;

#elif defined(__linux__) && __linux__

#include <pthread.h>

#define TLK_THREAD_INVALID            NULL

typedef pthread_t                     tlk_thread_t;
typedef void *                        tlk_thread_args;
typedef void *                        tlk_exit_t;

typedef void *(*tlk_thread_func)(void*);

#else

#error OS not supported

#endif


/*
 * Creates a new thread
 * Returns 0 on success, -1 on failure
 */
int tlk_thread_create (tlk_thread_t *thread, tlk_thread_func thread_routine, tlk_thread_args t_args);

/*
 * Detaches @thread from the current process
 * Returns 0 on success, -1 on failure
 */
int tlk_thread_detach (tlk_thread_t *thread);

/*
 * Wait for @thread termination
 * Returns 0 on success, -1 on failure
 */
int tlk_thread_join (tlk_thread_t *thread, void **exit_code);

/*
 * Terminates the current thread with @exit_code
 * Returns nothing
 */
void tlk_thread_exit (tlk_exit_t exit_code);

#endif
