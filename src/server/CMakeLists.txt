set (headers ../../include/)

set (server_sources server.c "${headers}/server.h")
set (server_libs tlk_sockets tlk_threads tlk_semaphores users msg_queue util)

add_executable (server ${server_sources})
target_link_libraries (server ${server_libs})
set_target_properties( server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)