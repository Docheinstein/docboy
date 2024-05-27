# Args
add_subdirectory(args)

# TUI
add_subdirectory(tui)

# zlib
set(ZLIB_BUILD_EXAMPLES OFF)
set(SKIP_INSTALL_ALL ON)
set(RENAME_ZCONF OFF)
add_subdirectory(zlib)

# Explicitly set ZLIB::ZLIB as an alias for zlibstatic so that libpng will use this.
add_library(ZLIB::ZLIB ALIAS zlibstatic)

# libpng
# Copy prebuilt pnglibconf.h into libpng source directory.
# We must to do so even if it is available system wise because we want to
# statically link the zlib library we have built; otherwise the libpng might
# complain because of zlib's version mismatch.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng/scripts/pnglibconf.h.prebuilt
           ${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h)

set(PNG_SHARED OFF CACHE BOOL "Build libpng as a shared library")
set(PNG_STATIC ON CACHE BOOL "Build libpng as a static library")
set(PNG_TESTS OFF CACHE BOOL "Build the libpng tests")
set(PNG_TOOLS OFF CACHE BOOL "Build the libpng tools")
add_subdirectory(libpng)

# SDL
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)

set(SDL_AUDIO OFF)
set(SDL_VIDEO ON)
set(SDL_RENDER ON)
set(SDL_JOYSTICK ON)
set(SDL_HAPTIC OFF)
set(SDL_HIDAPI ON)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)

add_subdirectory(SDL)

# nativefiledialog
# GTK3 is required by nativefiledialog on Linux: we don't build and link nfd at all if it's missing.
set(NFD_BUILD_TESTS OFF CACHE BOOL "Build tests for nfd")
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
    add_subdirectory(nativefiledialog)
else ()
    set(ENABLE_NFD OFF PARENT_SCOPE)
endif()

# Catch2
if (BUILD_TESTS)
    add_subdirectory(catch2)
endif()
