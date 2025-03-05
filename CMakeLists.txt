cmake_minimum_required(VERSION 3.24)
project(docboy)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Build static libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries instead of static")

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_DEVTOOLS "Build devtools" OFF)
option(BUILD_NOGUI_FRONTEND "Build NoGUI frontend" OFF)
option(BUILD_SDL_FRONTEND "Build SDL frontend" OFF)
option(BUILD_LIBRETRO_CORE "Build libretro core" OFF)
option(BUILD_LIBRETRO_CORE_ANDROID "Build libretro core for Android" OFF)

option(NDK_PATH "NDK Path for Android build" "")

option(ENABLE_DEBUGGER "Enable debugger" OFF)
option(ENABLE_BOOTROM "Enable boot rom" OFF)
option(ENABLE_AUDIO "Enable audio" ON)
option(ENABLE_CGB "Enable GameBoy Color support" ON)
option(ENABLE_TWO_PLAYERS_MODE "Enable two players mode (serial link)" ON)

option(ENABLE_ASSERTS "Enable asserts" OFF)

# Build in Release mode by default
if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (NOT BUILD_NOGUI_FRONTEND AND NOT BUILD_SDL_FRONTEND AND NOT BUILD_LIBRETRO_CORE AND NOT BUILD_LIBRETRO_CORE_ANDROID)
    set(BUILD_SDL_FRONTEND ON)
endif ()

if (BUILD_NOGUI_FRONTEND)
    message(STATUS "Frontend: NoGUI")
endif()

if (BUILD_SDL_FRONTEND)
    message(STATUS "Frontend: SDL")
endif()

if (BUILD_LIBRETRO_CORE)
    message(STATUS "Frontend: libretro core")
endif()

if (BUILD_LIBRETRO_CORE)
    message(STATUS "Frontend: libretro core (Android)")
endif()

message(STATUS "Feature: Debugger: ${ENABLE_DEBUGGER}")
message(STATUS "Feature: Boot room: ${ENABLE_BOOTROM}")
message(STATUS "Feature: Audio: ${ENABLE_AUDIO}")
message(STATUS "Feature: GameBoy Color: ${ENABLE_CGB}")
message(STATUS "Feature: Two players: ${ENABLE_TWO_PLAYERS_MODE}")

if (BUILD_LIBRETRO_CORE_ANDROID)
    add_subdirectory(src/frontend/libretro/android)
    return() # android compiles by itself with ndk-build
endif ()

include(CheckIncludeFile)

add_compile_options(-fPIC -Wall)

if (MSVC)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor /bigobj)
endif ()

if (ENABLE_ASSERTS)
    add_compile_definitions(ENABLE_ASSERTS)
endif()

check_include_file("unistd.h" HAS_UNISTD)
if (HAS_UNISTD)
    add_compile_definitions(POSIX)
endif()

if (WIN32)
    add_compile_definitions(WINDOWS)
endif()

if (BUILD_TESTS)
    enable_testing()
    add_compile_definitions(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

add_subdirectory(external)
add_subdirectory(src)


if (BUILD_DEVTOOLS)
    add_subdirectory(devtools)
endif()

