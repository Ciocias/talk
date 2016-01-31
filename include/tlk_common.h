#ifndef TLK_COMMON_H
#define TLK_COMMON_H

#define DEBUG                1
#define LOG                  0

#define DEFAULT_IP           "127.0.0.1"
#define DEFAULT_PORT         3000

#define MAX_CONN_QUEUE       16
#define MAX_USERS            16

#define MSG_SIZE             1024
#define NICKNAME_SIZE        10
#define QUEUE_SIZE           1024

#define WELCOME_MSG          "Welcome to Talk, user %s!"

#define MSG_DELIMITER_CHAR   '|'
#define COMMAND_CHAR         '/'

#define HELP_COMMAND         "help"
#define JOIN_COMMAND         "join"
#define LIST_COMMAND         "list"
#define TALK_COMMAND         "talk"
#define QUIT_COMMAND         "quit"

#define HELP_MSG             "%c%s             -> Show this help"
#define LIST_MSG             "%c%s             -> List current users"
#define TALK_MSG             "%c%s <nickname>  -> Start a talk with <nickname>"
#define QUIT_MSG             "%c%s             -> Quit session"

/* TMP */
#define JOIN_FAILED          "Join Error"
#define JOIN_SUCCESS         "Join Success"
#define REGISTER_FAILED      "Register Error"

#endif
