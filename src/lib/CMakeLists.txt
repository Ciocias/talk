set (headers ../../include/)

set (lib_semaphores_sources tlk_semaphores.c "${headers}/tlk_semaphores.h")
add_library (tlk_semaphores ${lib_semaphores_sources})

set (lib_sockets_sources tlk_sockets.c "${headers}/tlk_sockets.h")
add_library (tlk_sockets ${lib_sockets_sources})
if (WIN32 AND CMAKE_SYSTEM_VERSION)
  target_link_libraries (tlk_sockets wsock32 ws2_32 )
endif ()

find_package (Threads)
set (lib_threads_sources tlk_threads.c "${headers}/tlk_threads.h")
add_library (tlk_threads ${lib_threads_sources})
target_link_libraries (tlk_threads ${CMAKE_THREAD_LIBS_INIT})

set (lib_msg_queue_sources msg_queue.c "${headers}/msg_queue.h")
set (lib_msg_queue_libs tlk_semaphores)
add_library (msg_queue ${lib_msg_queue_sources})
target_link_libraries (msg_queue ${lib_msg_queue_libs})

set (lib_util_sources util.c "${headers}/util.h")
set (lib_util_libs tlk_sockets users msg_queue)
add_library (util ${lib_util_sources})
target_link_libraries (util ${lib_util_libs})

set (lib_users_sources users.c "${headers}/users.h")
set (lib_users_libs tlk_sockets tlk_threads tlk_semaphores msg_queue)
add_library (users ${lib_users_sources})
target_link_libraries (users ${lib_users_libs})
