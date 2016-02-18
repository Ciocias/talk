set (headers ../../include/)

set (server_sources tlk_server.c "${headers}/tlk_server.h")
set (server_libs tlk_sockets tlk_threads tlk_semaphores tlk_users tlk_msg_queue tlk_util)

add_executable (tlk_server ${server_sources})
target_link_libraries (tlk_server ${server_libs})
