cmake_minimum_required(VERSION 3.24)
project(gible)

set(CMAKE_CXX_STANDARD 20)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(tests)