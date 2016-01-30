# TODO list

## General

- Review and clean libs code
- Write working server code
- Write working client code


## Client

- initialize client data
 + parse command line arguments
 + setup signal handlers
 + setup socket
- client_main_loop()
 + connect() to server
 + start chatting
- cleanup and close


## Server

- initialize server data
 + parse command line arguments
 + setup signal handlers
 + setup socket
 + setup concurrent data structures
- server_main_loop()
 + block on accept()
 + launch new thread for every client_to_server session
- client_to_server session
 + send help and welcome message
 + handles server commands available to clients
 + launch new thread for a client_to_client session
- client_to_client session
 + handles messaging between clients
- cleanup and close
