set (headers ../../include/)

set (server_sources tlk_server.c "${headers}/tlk_server.h")
set (server_libs tlk_util tlk_users tlk_msg_queue tlk_sockets tlk_semaphores tlk_linked_list tlk_threads)

add_executable (tlk_server ${server_sources})
target_link_libraries (tlk_server ${server_libs})
