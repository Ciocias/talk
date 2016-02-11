set (headers ../../include/)

set (client_sources tlk_client.c "${headers}/tlk_client.h")
set (client_libs tlk_util tlk_sockets tlk_threads tlk_msg_queue)

add_executable (tlk_client ${client_sources})
target_link_libraries (tlk_client ${client_libs})
