cmake_minimum_required(VERSION 3.24)
project(docboy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(ENABLE_DEBUGGER "Add debugger support" ON)
option(BUILD_NOGUI_FRONTEND "Build with NoGUI frontend" ON)
option(BUILD_SDL_FRONTEND "Build with SDL frontend" OFF)
option(BUILD_QT_FRONTEND "Build with Qt frontend" OFF)
option(BUILD_TESTS "Build tests" OFF)

add_compile_options("-Wall")

add_subdirectory(external)
add_subdirectory(src)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()