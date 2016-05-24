cmake_minimum_required (VERSION 2.6)
project (talk)

if (CMAKE_COMPILER_IS_GNUCC)
    set (CMAKE_C_FLAGS "-Wall -Wextra")
endif()

add_subdirectory ("${PROJECT_SOURCE_DIR}/src/lib")
add_subdirectory ("${PROJECT_SOURCE_DIR}/src/client")
add_subdirectory ("${PROJECT_SOURCE_DIR}/src/server")
