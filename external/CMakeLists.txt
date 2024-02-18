# Args
add_subdirectory(args)

# TUI
add_subdirectory(tui)

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

# SDL_image

# SDL_Image with its dependencies it's a mess to build on Windows.
# Disable it since we lose just the screenshot as a feature.
if (NOT WIN32)
    add_subdirectory(SDL_image)
endif()