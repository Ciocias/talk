#ifndef TLK_COMMON_H
#define TLK_COMMON_H

#define DEBUG                1
#define LOG                  1

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

#define HELP_CMD         	 "help"
#define JOIN_COMMAND         "join"
#define LIST_COMMAND         "list"
#define TALK_COMMAND         "talk"
#define QUIT_COMMAND         "quit"
#define CLOSE_COMMAND        "close"

#define HELP_MSG             "%c%s             -> Show this help"
#define LIST_MSG             "%c%s             -> List current users"
#define TALK_MSG             "%c%s <nickname>  -> Start a talk with <nickname>"
#define CLOSE_MSG            "%c%s            -> Close chat session"
#define QUIT_MSG             "%c%s             -> Quit"

/* TMP */
#define JOIN_FAILED          "Join Error"
#define JOIN_SUCCESS         "Join Success"
#define REGISTER_FAILED      "Register Error"
#define USER_NOT_FOUND       "Unable to find user"
#define BEGIN_CHAT_MSG       "You're now talking with %s!"
#define NO_NICKNAME          "Error: no nickname specified"

#endif
