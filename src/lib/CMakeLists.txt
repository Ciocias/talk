set (headers ../../include/)

set (lib_linked_list_sources tlk_linked_list.c "${headers}/tlk_linked_list.h")
add_library (tlk_linked_list ${lib_linked_list_sources})

set (lib_min_heap_sources tlk_min_heap.c "${headers}/tlk_min_heap.h")
add_library (tlk_min_heap ${lib_min_heap_sources})

set (lib_semaphores_sources tlk_semaphores.c "${headers}/tlk_semaphores.h")
add_library (tlk_semaphores ${lib_semaphores_sources})

set (lib_sockets_sources tlk_sockets.c "${headers}/tlk_sockets.h")
add_library (tlk_sockets ${lib_sockets_sources})
target_link_libraries (tlk_sockets wsock32 ws2_32 )

find_package (Threads)
set (lib_threads_sources tlk_threads.c "${headers}/tlk_threads.h")
add_library (tlk_threads ${lib_threads_sources})
target_link_libraries (tlk_threads ${CMAKE_THREAD_LIBS_INIT})

set (lib_msg_queue_sources tlk_msg_queue.c "${headers}/tlk_msg_queue.h")
set (lib_msg_queue_libs tlk_min_heap tlk_semaphores tlk_users)
add_library (tlk_msg_queue ${lib_msg_queue_sources})
target_link_libraries (tlk_msg_queue ${lib_msg_queue_libs})

set (lib_util_sources tlk_util.c "${headers}/tlk_util.h")
set (lib_util_libs tlk_sockets tlk_users)
add_library (tlk_util ${lib_util_sources})
target_link_libraries (tlk_util ${lib_util_libs})

set (lib_users_sources tlk_users.c "${headers}/tlk_users.h")
set (lib_users_libs tlk_linked_list tlk_sockets tlk_threads tlk_semaphores)
add_library (tlk_users ${lib_users_sources})
target_link_libraries (tlk_users ${lib_users_libs})
