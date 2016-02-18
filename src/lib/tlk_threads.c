#include "../../include/tlk_threads.h"

/*
 * Creates a new thread
 * Returns 0 on success, -1 on failure
 */
int tlk_thread_create(tlk_thread_t *thread, tlk_thread_func thread_routine, tlk_thread_args t_args) {
  int ret = 0;

#if defined(_WIN32) && _WIN32

  *thread = CreateThread(NULL, 0, thread_routine, t_args, 0, NULL);

  ret = (*thread != NULL ? 0 : -1);

#elif defined(__linux__) && __linux__

  ret = pthread_create(thread, NULL, thread_routine, t_args);
  if (ret) return -1;

#endif

  return ret;
}

/*
 * Detaches @thread from the current process
 * Returns 0 on success, -1 on failure
 */
int tlk_thread_detach(tlk_thread_t *thread) {

  int ret = 0;

#if defined(_WIN32) && _WIN32

  ret = (CloseHandle(*thread) != 0 ? 0 : -1);

#elif defined(__linux__) && __linux__

  ret = pthread_detach(*thread);
  if (ret) return -1;

#endif

  return ret;
}

/*
 * Wait for @thread termination
 * Returns 0 on success, -1 on failure
 */
int tlk_thread_join(tlk_thread_t *thread, void **exit_code) {

  int ret = 0;

#if defined(_WIN32) && _WIN32

  ret = (WaitForSingleObject((HANDLE) *thread, INFINITE) != WAIT_FAILED ? 0 : -1);
  if (ret) return ret;

  *exit_code = ret;

  /* TODO: Find a correct way to get exitcode from threads on windows */  
  /*
  ret = (GetExitCodeThread((HANDLE) *thread, (LPDWORD) *exit_code) != 0 ? 0 : -1);
  */
#elif defined(__linux__) && __linux__

  ret = pthread_join(*thread, exit_code);
  if (ret) return -1;

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

  return;
}
