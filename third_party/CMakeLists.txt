# Args
add_subdirectory(args)

# TUI
add_subdirectory(tui)

# zlib
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "Enable Zlib Examples as tests")
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "Enable building zlib shared library")
set(ZLIB_BUILD_STATIC ON CACHE BOOL "Enable building zlib static library")
set(ZLIB_BUILD_MINIZIP OFF CACHE BOOL "Enable building libminizip contrib library")
set(ZLIB_INSTALL OFF CACHE BOOL "Enable installation of zlib")
if(WIN32)
    set(ZLIB_INSTALL_COMPAT_DLL OFF CACHE BOOL "Install a copy as zlib1.dll")
endif(WIN32)

add_subdirectory(zlib)

# Explicitly set ZLIB::ZLIB as an alias for zlibstatic so that libpng will use this.
add_library(ZLIB::ZLIB ALIAS zlibstatic)

# libpng
set(PNG_SHARED OFF CACHE BOOL "Build libpng as a shared library")
set(PNG_STATIC ON CACHE BOOL "Build libpng as a static library")
set(PNG_FRAMEWORK OFF CACHE BOOL "Build libpng as a framework bundle")
set(PNG_TESTS OFF CACHE BOOL "Build the libpng tests")
set(PNG_TOOLS OFF CACHE BOOL "Build the libpng tools")
set(SKIP_INSTALL_ALL ON)

# Force libpng to use our zlib for generation of the pnglibconf.h.
set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib")
set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${ZLIB_INCLUDE_DIR}")

add_subdirectory(libpng)

# SDL
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)

set(SDL_AUDIO ${ENABLE_AUDIO})
set(SDL_VIDEO ON)
set(SDL_RENDER ON)
set(SDL_JOYSTICK ON)
set(SDL_HAPTIC OFF)
set(SDL_HIDAPI ON)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_DUMMYAUDIO OFF)
set(SDL_DUMMYCAMERA OFF)
set(SDL_DUMMYVIDEO OFF)

add_subdirectory(SDL)

# nativefiledialog
# GTK3 is required by nativefiledialog on Linux: we don't build and link nfd at all if it's missing.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build a shared library instead of static")
set(NFD_BUILD_TESTS OFF CACHE BOOL "Build tests for nfd")
set(NFD_BUILD_SDL2_TESTS OFF CACHE BOOL "Build SDL2 tests for nfd")
set(NFD_INSTALL OFF CACHE BOOL "Generate install target for nfd")

set(ENABLE_NFD ON)

if (UNIX AND NOT APPLE)
    find_package(PkgConfig)
    pkg_check_modules(GTK3 gtk+-3.0)
    if (NOT GTK3_FOUND)
        set(ENABLE_NFD OFF)
    endif ()
endif ()

if (ENABLE_NFD)
    set(ENABLE_NFD ON PARENT_SCOPE)
    add_subdirectory(nfd)
else ()
    set(ENABLE_NFD OFF PARENT_SCOPE)
endif()

# Catch2
if (BUILD_TESTS)
    add_subdirectory(catch2)
    add_subdirectory(simdjson)
endif()
