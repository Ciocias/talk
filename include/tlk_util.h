#ifndef TLK_UTIL_H
#define TLK_UTIL_H

#include "tlk_sockets.h"
#include "tlk_msg_queue.h"
#include "tlk_users.h"

#if defined(_WIN32) && _WIN32

#define _WINSOCKAPI_
#include <Winsock2.h>

#elif defined(__linux__) && __linux__
#include <netinet/in.h>
#endif

/*
 * Parse network-ordered port number from @input string and put it in @output
 * Returns 0 on success, -1 on failure
 */
int parse_port_number (const char *input, unsigned short *output);

/*
 * Send @msg on @socket
 * Returns the number of bytes sent on success, TLK_SOCKET_ERROR on failure
 */
int send_msg (tlk_socket_t socket, const char *msg);

/*
 * Receive @buf_len bytes from @socket and put contents inside @buf
 * Returns:
 *  Number of bytes read on success
 *  TLK_CONN_CLOSED if the endpoint has closed gracefully
 *  TLK_SOCKET_ERROR on failure
 */
int recv_msg (tlk_socket_t socket, char *buf, int buf_len);

/*
 * Parse nickname from join message
 * Returns 0 on success, -1 on failure
 */
int parse_join_msg (char *msg, size_t msg_len, char *nickname);

/*
 * Send help message and commands list through @socket
 * Returns nothing
 */
void send_help (tlk_socket_t socket);

/*
 * Send users @list through @socket as a list of nicknames
 * Returns nothing
 */
void send_list (unsigned int limit, tlk_socket_t socket, tlk_user_t *list[MAX_USERS]);

/*
 * Send UNKNOWN_CMD_MSG through @socket
 * Returns nothing
 */
int send_unknown (tlk_socket_t socket);

/*
 * Enqueue a DIE_MSG for @user into @queue
 * Returns 0 on success, -1 on failure
 */
int terminate_receiver (tlk_user_t *user, tlk_queue_t *queue);

/*
 * Try to establish a talk session
 * Returns:
 *   0 on success,
 *   -1 on enqueuing failure,
 *   1 if listener is NULL or TALKING
 */
int talk_session (tlk_user_t *user, tlk_queue_t *queue);

/*
 * Setup a new tlk_message_t with @msg as content and put it in @queue
 * Returns 0 on success, -1 on failure
 */
int pack_and_send_msg (int id, tlk_user_t *sender, tlk_user_t *receiver, char *msg, tlk_queue_t *queue);

#endif /* TLK_UTIL_H */
