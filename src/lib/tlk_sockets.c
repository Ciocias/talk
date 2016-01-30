#include "../../include/tlk_sockets.h"

/*
 * Creates a new @type socket with @addr_fam and @protocol
 * Returns the descriptor of the newly created socket, TLK_SOCKET_INVALID on error
 */
tlk_socket_t tlk_socket_create(int addr_fam, int type, int protocol) {

  tlk_socket_t socket_desc = TLK_SOCKET_INVALID;

#if defined(_WIN32) && _WIN32

  int ret;
  WSADATA wsaData;

  /* Initialize winsocket 2.2 library */
  ret = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (ret != 0) {
    return socket_desc;
  }

  socket_desc = socket(addr_fam, type, protocol);
  if (socket_desc == TLK_SOCKET_INVALID) {
    WSACleanup();
    return socket_desc;
  }

#elif defined(__linux__) && __linux__

  socket_desc = socket(addr_fam, type, protocol);

#endif

  return socket_desc;
}

/*
 * Binds @socket_desc with @addr
 * Returns 0 on success, propagates errors on fail
 */
int tlk_socket_bind(tlk_socket_t socket_desc, const struct sockaddr *addr, int addr_len) {

  int ret;

/*#if defined(_WIN32) && _WIN32
  ret = bind(socket_desc, addr, addr_len);
#elif defined(__linux__) && __linux__*/
  ret = bind(socket_desc, addr, addr_len);
/*#endif*/

  return ret;
}

/*
 * Set the @socket_desc to listening mode
 * Returns 0 on success, propagates errors on fail
 */
int tlk_socket_listen(tlk_socket_t socket_desc, int backlog) {

  int ret;

/*#if defined(_WIN32) && _WIN32
  ret = listen(socket_desc, backlog);
#elif defined(__linux__) && __linux__*/
  ret = listen(socket_desc, backlog);
/*#endif*/

  return ret;
}

/*
 * Blocks on accept until someone tries to connect, then it fills @addr with the other endpoint sockaddr address
 * Returns 0 on success, propagates errors on fail
 */
tlk_socket_t tlk_socket_accept(tlk_socket_t socket_desc, struct sockaddr *addr, int addr_len) {

  tlk_socket_t other_desc = TLK_SOCKET_INVALID;

#if defined(_WIN32) && _WIN32

  other_desc = accept(socket_desc, addr, &addr_len);

#elif defined(__linux__) && __linux__

  other_desc = accept(socket_desc, addr, (socklen_t *) &addr_len);

#endif

  return other_desc;
}

/*
 * Try to connect to @addr host on @socket_desc
 * Returns 0 on success, propagates errors on fail
 */
int tlk_socket_connect(tlk_socket_t socket_desc, const struct sockaddr *addr, int addr_len) {

  int ret;

#if defined(_WIN32) && _WIN32
  ret = connect(socket_desc, addr, addr_len);
#elif defined(__linux__) && __linux__
  ret = connect(socket_desc, addr, (socklen_t) addr_len);
#endif

  return ret;
}

/*
 * Stop performing @mode operations on @socket_desc
 * Returns 0 on success, propagates errors on fail
 */
int tlk_socket_shutdown(tlk_socket_t socket_desc, int mode) {

  int ret/*, correct_mode*/;
/*
#if defined(_WIN32) && _WIN32

  if (mode == TLK_SOCKET_R) {
    correct_mode = SD_RECEIVE;
  } else if (mode == TLK_SOCKET_W) {
    correct_mode = SD_SEND;
  } else if (mode == TLK_SOCKET_RW) {
    correct_mode = SD_BOTH;
  } else {
    return -1;
  }

#elif defined(__linux__) && __linux__

  if (mode == TLK_SOCKET_R) {
    correct_mode = SHUT_RD;
  } else if (mode == TLK_SOCKET_W) {
    correct_mode = SHUT_WR;
  } else if (mode == TLK_SOCKET_RW) {
    correct_mode = SHUT_RDWR;
  } else {
    return -1;
  }

#endif
*/
  ret = shutdown(socket_desc, /*correct_*/mode);
  return ret;
}

/*
 * Close @socket_desc
 * Returns 0 on success, propagates errors on fail
 */
int tlk_socket_close(tlk_socket_t socket_desc) {

  int ret;
#if defined(_WIN32) && _WIN32

  ret = closesocket(socket_desc);
  WSACleanup();

#elif defined(__linux__) && __linux__

  ret = close(socket_desc);

#endif

  return ret;
}
