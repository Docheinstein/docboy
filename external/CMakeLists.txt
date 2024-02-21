# Args
add_subdirectory(args)

# TUI
add_subdirectory(tui)

# libpng
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