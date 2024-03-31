add_executable(docboy-sdl)

target_sources(docboy-sdl PRIVATE
        main.cpp
        window.h
        window.cpp
        glyphs.h
        glyphs.cpp
)

target_link_libraries(docboy-sdl PRIVATE docboy utils extra args SDL3-static png_static)