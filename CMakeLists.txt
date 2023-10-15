cmake_minimum_required(VERSION 3.24)
project(docboy)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_NOGUI_FRONTEND "Build NoGUI frontend" OFF)
option(BUILD_SDL_FRONTEND "Build SDL frontend" OFF)
option(BUILD_LIBRETRO_CORE "Build libretro core" OFF)
option(BUILD_LIBRETRO_CORE_ANDROID "Build libretro core for Android" OFF)

option(NDK_PATH "NDK Path for Android build" "")

option(ENABLE_DEBUGGER "Enable debugger" OFF)
option(ENABLE_SERIAL "Enable serial link" OFF)
option(ENABLE_BOOTROM "Enable boot rom" OFF)

# Build in Release mode by default
if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (NOT BUILD_NOGUI_FRONTEND AND NOT BUILD_SDL_FRONTEND AND NOT BUILD_LIBRETRO_CORE AND NOT BUILD_LIBRETRO_CORE_ANDROID)
    message(FATAL_ERROR
            "Please specify at least a frontend to build.\n"
            "Possible options are:\n"
            "> BUILD_NOGUI_FRONTEND\n"
            "> BUILD_SDL_FRONTEND\n"
            "> BUILD_LIBRETRO_CORE\n"
            "> BUILD_LIBRETRO_CORE_ANDROID"
    )
endif ()

if (BUILD_LIBRETRO_CORE_ANDROID)
    add_subdirectory(src/frontend/libretro/android)
    return() # android compiles by itself with ndk-build
endif ()

include(CheckIncludeFile)

add_compile_options(-fPIC -Wall)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ENABLE_ASSERTS)
endif()

check_include_file("unistd.h" HAS_UNISTD)
if (HAS_UNISTD)
    add_compile_definitions(POSIX)
endif()

add_subdirectory(external)
add_subdirectory(src)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()