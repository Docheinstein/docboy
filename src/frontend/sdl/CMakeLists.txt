add_executable(docboy-sdl)

target_sources(docboy-sdl PRIVATE
        glyphs.cpp
        glyphs.h
        main.cpp
        window.cpp
        window.h
)

target_link_libraries(docboy-sdl PRIVATE docboy utils extra args SDL3-static png_static)