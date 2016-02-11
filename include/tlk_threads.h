#ifndef TLK_THREADS_H
#define TLK_THREADS_H

typedef int tlk_thread_id; /* TEMP */

#if defined(_WIN32) && _WIN32
#define _WINSOCKAPI_

#include <Windows.h>

typedef HANDLE tlk_thread_t;

/* tmp */
typedef LPTHREAD_START_ROUTINE tlk_thread_func;
typedef LPVOID tlk_thread_args;
typedef DWORD tlk_exit_t;

#elif defined(__linux__) && __linux__

#include <pthread.h>

typedef pthread_t tlk_thread_t;
typedef void *(*tlk_thread_func)(void*);
typedef void *tlk_thread_args;
typedef void *tlk_exit_t;

#else
#error OS not supported
#endif

/*
#if defined(_WIN32) && _WIN32

int tlk_thread_create(tlk_thread_t *thread, LPTHREAD_START_ROUTINE thread_routine, LPVOID thread_args);

#elif defined(__linux__) && __linux__

int tlk_thread_create(tlk_thread_t *thread, void *(*thread_routine)(void*), void *thread_args);

#endif
*/

/*
 * Creates new thread and makes @thread point to it, passing @thread_routine and @t_args to the functions beneath
 * Returns 0 on success, propagates errors on fail
 */
int tlk_thread_create(tlk_thread_t *thread, tlk_thread_func thread_routine, tlk_thread_args t_args);

/*
 * Detaches @thread from the current process
 * Returns 0 on success, propagates errors on fail
 */
int tlk_thread_detach(tlk_thread_t *thread);

/*
 * Perform a join operation on @thread and puts exit value in @exit_code, if any
 * Returns 0 on success, propagates errors on fail
 */
int tlk_thread_join(tlk_thread_t *thread, void *exit_code);

/*
 * Terminates the current thread with @exit_code
 * Returns nothing
 */
void tlk_thread_exit(tlk_exit_t exit_code);

#endif
