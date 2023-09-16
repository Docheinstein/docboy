cmake_minimum_required(VERSION 3.24)
project(docboy)
include(CheckIncludeFile)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(ENABLE_DEBUGGER "Add debugger support" ON)
option(ENABLE_PROFILER "Enable the profiler" OFF)

# TODO: Set everything off but default target (BUILD_QT_FRONTEND probably)
option(BUILD_NOGUI_FRONTEND "Build with NoGUI frontend" ON)
option(BUILD_SDL_FRONTEND "Build with SDL frontend" ON)
option(BUILD_QT_FRONTEND "Build with Qt frontend" OFF)
option(BUILD_LIBRETRO_CORE "Build libretro core" ON)
option(BUILD_TESTS "Build tests" OFF)

add_compile_options(-fPIC -Wall)

check_include_file("unistd.h" HAS_UNISTD)
if (HAS_UNISTD)
    add_compile_definitions(POSIX)
endif()

add_subdirectory(external)
add_subdirectory(src)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()