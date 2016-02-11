#include "../../include/tlk_threads.h"

/*
 * Creates new thread and makes @thread point to it, passing @thread_routine and @t_args to the functions beneath
 * Returns 0 on success, propagates errors on fail
 */
int tlk_thread_create(tlk_thread_t *thread, tlk_thread_func thread_routine, tlk_thread_args t_args) {
  int ret;

#if defined(_WIN32) && _WIN32

  *thread = CreateThread(NULL, 0, thread_routine, t_args, 0, NULL);

  ret = (*thread != NULL ? 0:1);

#elif defined(__linux__) && __linux__

  ret = pthread_create(thread, NULL, thread_routine, t_args);

#endif

  return ret;
}

/*
 * Detaches @thread from the current process
 * Returns 0 on success, propagates errors on fail
 */
int tlk_thread_detach(tlk_thread_t thread) {

  int ret;

#if defined(_WIN32) && _WIN32

  ret = (CloseHandle(thread) != FALSE ? 0:1);

#elif defined(__linux__) && __linux__

  ret = pthread_detach(thread);

#endif

  return ret;
}

/*
 * Perform a join operation on @thread and puts exit value in @exit_code, if any
 * Returns 0 on success, propagates errors on fail
 */
int tlk_thread_join(tlk_thread_t thread, void *exit_code) {

  int ret;

#if defined(_WIN32) && _WIN32

  ret = (WaitForSingleObject(thread, INFINITE) != WAIT_FAILED ? 0:1);

  if (ret) {
    return ret;
  } else {
    ret = (GetExitCodeThread(thread, exit_code) != FALSE ? 0:1);
  }

#elif defined(__linux__) && __linux__

  ret = pthread_join(thread, &exit_code);

#endif

  return ret;
}

/*
 * Terminates the current thread with @exit_code
 * Returns nothing
 */
void tlk_thread_exit(tlk_exit_t exit_code) {
#if defined(_WIN32) && _WIN32

  ExitThread(exit_code);

#elif defined(__linux__) && __linux__

  pthread_exit(exit_code);

#endif

}
