#ifndef TLK_COMMON_H
#define TLK_COMMON_H

#define LOG                  1

#define DEFAULT_IP           "127.0.0.1"
#define DEFAULT_PORT         3000

#define MAX_CONN_QUEUE       16
#define MAX_USERS            16

#define MSG_SIZE             1024
#define NICKNAME_SIZE        10
#define QUEUE_SIZE           1024

#define WELCOME_MSG          "Welcome to Talk, user %s!"

#define COMMAND_CHAR         '/'
#define HELP_CMD         	   "help"
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

#define JOIN_FAILED          "Join Error"
#define JOIN_SUCCESS         "Join Success"
#define REGISTER_FAILED      "Register Error"
#define MAX_USERS_ERROR_MSG  "Max users limit reached"
#define NICKNAME_ERROR_MSG   "Nickname already taken"
#define USER_NOT_FOUND       "Unable to find user"
#define BEGIN_CHAT_MSG       "You're now talking with %s!"
#define NO_NICKNAME          "Error: no nickname specified"
#define CLOSE_TALK_MSG       "No more talking"
#define DIE_MSG              "die"
#define BRK_DIE_MSG          "kill"
#define UNKNOWN_CMD_MSG      "Unknown command, type %c%s for a list of available commands"
#define IDLE_MSG             "You are idle"
#define CLOSE_CHAT_MSG		   "Your current chat session is closed"

#endif /* TLK_COMMON_H */
