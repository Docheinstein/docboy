add_executable(docboy-sdl)

target_sources(docboy-sdl PRIVATE
    components/glyphs.cpp
    components/glyphs.h
    components/menu.cpp
    components/menu.h
    components/text.cpp
    components/text.h
    controllers/corecontroller.cpp
    controllers/corecontroller.h
    controllers/maincontroller.h
    controllers/navcontroller.cpp
    controllers/navcontroller.h
    controllers/uicontroller.cpp
    controllers/uicontroller.h
    main.cpp
    screens/controloptionsscreen.cpp
    screens/controloptionsscreen.h
    screens/gamescreen.cpp
    screens/gamescreen.h
    screens/graphicsoptionsscreen.cpp
    screens/graphicsoptionsscreen.h
    screens/helpscreen.cpp
    screens/helpscreen.h
    screens/launcherscreen.cpp
    screens/launcherscreen.h
    screens/mainscreen.cpp
    screens/mainscreen.h
    screens/menuscreen.cpp
    screens/menuscreen.h
    screens/optionsscreen.cpp
    screens/optionsscreen.h
    screens/screen.cpp
    screens/screen.h
    screens/screenstack.cpp
    screens/screenstack.h
    screens/shortcutsscreen.cpp
    screens/shortcutsscreen.h
    utils/rompicker.cpp
    utils/rompicker.h
    utils/textures.cpp
    utils/textures.h
    window.cpp
    window.h
)

if (ENABLE_DEBUGGER)
    target_sources(docboy-sdl PRIVATE
            controllers/debuggercontroller.cpp
            controllers/debuggercontroller.h
    )
endif ()

if (ENABLE_NFD)
    add_compile_definitions(NFD)
    target_link_libraries(docboy-sdl PRIVATE nfd)
endif ()

target_link_libraries(docboy-sdl PRIVATE docboy utils extra args SDL3-static png_static)

target_include_directories(docboy-sdl PRIVATE .)
