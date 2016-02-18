#include "../../include/tlk_sockets.h"
#include <stdio.h>
#if defined(_WIN32) && _WIN32

/*
* (Windows only)
* Initiates use of the Winsock DLL by a process (ignores LPWSADATA parameter)
* Returns 0 on success, TLK_SOCKET_ERROR on failure
*/
int tlk_socket_init() {
	int ret = 0;
	WSADATA wsaData;

	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret) return TLK_SOCKET_ERROR;

	return ret;
}

/*
* (Windows only)
* Terminates use of the Winsock DLL by a process
* Returns 0 on success, TLK_SOCKET_ERROR on failure
*/
int tlk_socket_cleanup() {

	int ret = 0;

	ret = WSACleanup();
	if (ret) return TLK_SOCKET_ERROR;

	return ret;
}

#endif

/*
 * Creates a new @type socket with @addr_fam and @protocol
 * Returns the socket descriptor on success, TLK_SOCKET_INVALID on failure
 */
tlk_socket_t tlk_socket_create(int addr_fam, int type, int protocol) {

  tlk_socket_t socket_desc = TLK_SOCKET_INVALID;

#if defined(_WIN32) && _WIN32

  int ret;

  socket_desc = socket(addr_fam, type, protocol);
  if (socket_desc == INVALID_SOCKET) {
    return TLK_SOCKET_INVALID;
  }

#elif defined(__linux__) && __linux__

  socket_desc = socket(addr_fam, type, protocol);
  if (socket_desc == -1) {
    return TLK_SOCKET_INVALID;
  }

#endif

  return socket_desc;
}

/*
 * Binds @socket_desc with @addr
 * Returns 0 on success, TLK_SOCKET_ERROR on failure
 */
int tlk_socket_bind(tlk_socket_t socket_desc, const struct sockaddr *addr, int addr_len) {

  int ret;

  ret = bind(socket_desc, addr, addr_len);
  if (ret) {
    return TLK_SOCKET_ERROR;
  }

  return ret;
}

/*
 * Set the @socket_desc to listening mode
 * Returns 0 on success, TLK_SOCKET_ERROR on failure
 */
int tlk_socket_listen(tlk_socket_t socket_desc, int backlog) {

  int ret;

  ret = listen(socket_desc, backlog);
  if (ret) {
    return TLK_SOCKET_ERROR;
  }

  return ret;
}

/*
 * Blocks on accept until someone tries to connect, then it fills @addr with the other endpoint sockaddr address
 * Returns 0 on success, TLK_SOCKET_INVALID on failure
 */
tlk_socket_t tlk_socket_accept(tlk_socket_t socket_desc, struct sockaddr *addr, int addr_len) {

  tlk_socket_t other_desc = TLK_SOCKET_INVALID;

#if defined(_WIN32) && _WIN32

  other_desc = accept(socket_desc, addr, &addr_len);

  if (other_desc == INVALID_SOCKET) {
    return TLK_SOCKET_INVALID;
  }

#elif defined(__linux__) && __linux__

  other_desc = accept(socket_desc, addr, (socklen_t *) &addr_len);

  if (other_desc == -1) {
    return TLK_SOCKET_INVALID;
  }

#endif

  return other_desc;
}

/*
 * Try to connect to @addr host on @socket_desc
 * Returns 0 on success, TLK_SOCKET_ERROR on failure
 */
int tlk_socket_connect(tlk_socket_t socket_desc, const struct sockaddr *addr, int addr_len) {

  int ret;

#if defined(_WIN32) && _WIN32
  ret = connect(socket_desc, addr, addr_len);
#elif defined(__linux__) && __linux__
  ret = connect(socket_desc, addr, (socklen_t) addr_len);
#endif

  if (ret) {
    return TLK_SOCKET_ERROR;
  }

  return ret;
}

/*
 * Close @socket_desc
 * Returns 0 on success, TLK_SOCKET_ERROR on failure
 */
int tlk_socket_close(tlk_socket_t socket_desc) {

  int ret;
#if defined(_WIN32) && _WIN32

  ret = closesocket(socket_desc);
  if (ret) {
    return TLK_SOCKET_ERROR;
  }

#elif defined(__linux__) && __linux__

  ret = close(socket_desc);
  if (ret) {
    return TLK_SOCKET_ERROR;
  }

#endif

  return ret;
}
