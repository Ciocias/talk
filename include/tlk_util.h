#ifndef TLK_UTIL_H
#define TLK_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tlk_sockets.h"
#include "tlk_users.h"
#include "tlk_errors.h"

#if defined(_WIN32) && _WIN32
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
 * Print usage functions (server/client version)
 */
void usage_error_server (const char *prog_name);
void usage_error_client (const char *prog_name);

/*
 * Free @user socket and data structure
 * Exit thread on return
 */
void close_and_free_chat_session (tlk_user_t *user);

/*
 * Send @msg on @socket
 * Returns nothing
 */
int send_msg (tlk_socket_t socket, const char *msg);

/*
 * Receive @buf_len bytes from @socket and put contents inside @buf
 * Returns true bytes read
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
void send_users_list (tlk_socket_t socket, tlk_user_t *list[MAX_USERS]);

/*
 *
 *
 */
void parse_talkmsg_target(char msg[MSG_SIZE]);

/*
 *
 *
 */
tlk_message_t *talk_session (tlk_user_t *user, char msg[MSG_SIZE]);

int tlk_inet_pton(int af, const char *src, void *dst);

#endif
