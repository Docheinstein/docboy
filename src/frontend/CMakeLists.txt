if (BUILD_NOGUI_FRONTEND)
    add_subdirectory(nogui)
endif()

if (BUILD_QT_FRONTEND)
    add_subdirectory(qt)
endif()

if (BUILD_SDL_FRONTEND)
    add_subdirectory(sdl)
endif()