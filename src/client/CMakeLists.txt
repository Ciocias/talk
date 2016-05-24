set (headers ../../include/)

set (client_sources client.c "${headers}/client.h")
set (client_libs tlk_sockets tlk_threads util)

add_executable (client ${client_sources})
target_link_libraries (client ${client_libs})
set_target_properties( client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)