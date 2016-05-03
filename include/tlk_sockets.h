#ifndef TLK_SOCKETS_H
#define TLK_SOCKETS_H

#define TLK_SOCKET_INVALID            -1
#define TLK_CONN_CLOSED               -2

#define TLK_SOCKET_R                  0
#define TLK_SOCKET_W                  1
#define TLK_SOCKET_RW                 2

#if defined(_WIN32) && _WIN32

#define _WINSOCKAPI_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Winsock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#define TLK_SOCKET_ERRNO			  WSAGetLastError()

#define TLK_EINTR                     WSAEINTR
#define TLK_EAFNOSUPPORT              WSAEAFNOSUPPORT
#define TLK_SOCKET_ERROR              SOCKET_ERROR

typedef unsigned int tlk_socket_t;

#pragma comment(lib, "Ws2_32.lib")

#elif defined(__linux__) && __linux__

#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define TLK_SOCKET_ERRNO			  errno

#define TLK_EINTR                     EINTR
#define TLK_EAFNOSUPPORT              EAFNOSUPPORT
#define TLK_SOCKET_ERROR              -1

typedef int tlk_socket_t;

#else

#error OS not supported

#endif

#if defined(_WIN32) && _WIN32

/*
* (Windows only)
* Initiates use of the Winsock DLL by a process (ignores LPWSADATA parameter)
* Returns 0 on success, TLK_SOCKET_ERROR on failure
*/
int tlk_socket_init ();

/*
* (Windows only)
* Terminates use of the Winsock DLL by a process
* Returns 0 on success, TLK_SOCKET_ERROR on failure
*/
int tlk_socket_cleanup ();

#endif

/*
 * Creates a new @type socket with @addr_fam and @protocol
 * Returns the socket descriptor on success, TLK_SOCKET_INVALID on failure
 */
tlk_socket_t tlk_socket_create (int addr_fam, int type, int protocol);

/*
 * Binds @socket_desc with @addr
 * Returns 0 on success, TLK_SOCKET_ERROR on failure
 */
int tlk_socket_bind (tlk_socket_t socket_desc, const struct sockaddr *addr, int addr_len);

 /*
  * Set the @socket_desc to listening mode
  * Returns 0 on success, TLK_SOCKET_ERROR on failure
  */
int tlk_socket_listen (tlk_socket_t socket_desc, int backlog);

/*
 * Blocks on accept, then it fills @addr with the other endpoint sockaddr address
 * Returns 0 on success, TLK_SOCKET_INVALID on failure
 */
tlk_socket_t tlk_socket_accept (tlk_socket_t socket_desc, struct sockaddr *addr, int addr_len);

/*
 * Try to connect to @addr host on @socket_desc
 * Returns 0 on success, TLK_SOCKET_ERROR on failure
 */
int tlk_socket_connect (tlk_socket_t socket_desc, const struct sockaddr *addr, int addr_len);

/*
 * Close @socket_desc
 * Returns 0 on success, TLK_SOCKET_ERROR on failure
 */
int tlk_socket_close (tlk_socket_t socket_desc);

#endif /* TLK_SOCKETS_H */
