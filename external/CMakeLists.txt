# Args
add_subdirectory(args)

# TUI
add_subdirectory(tui)

# zlib
set(ZLIB_BUILD_EXAMPLES OFF)
add_subdirectory(zlib)
add_library(ZLIB::ZLIB ALIAS zlibstatic) # libpng uses ZLIB::ZLIB

# libpng
check_include_file("pnglibconf.h" HAS_PNGLIBCONF_H)
if (NOT HAS_PNGLIBCONF_H)
    # Copy prebuilt pnglibconf.h into libpng source directory if not available system-wise
      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng/scripts/pnglibconf.h.prebuilt
                 ${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h)
endif()

set(PNG_SHARED OFF CACHE BOOL "Build libpng as a shared library")
set(PNG_STATIC ON CACHE BOOL "Build libpng as a static library")
set(PNG_TESTS OFF CACHE BOOL "Build the libpng tests")
set(PNG_TOOLS OFF CACHE BOOL "Build the libpng tools")
set(SKIP_INSTALL_ALL ON)
add_subdirectory(libpng)

# SDL
set(SDL_STATIC ON)

set(SDL_AUDIO OFF)
set(SDL_VIDEO ON)
set(SDL_RENDER ON)
set(SDL_JOYSTICK ON)
set(SDL_HAPTIC OFF)
set(SDL_HIDAPI OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)

add_subdirectory(SDL)