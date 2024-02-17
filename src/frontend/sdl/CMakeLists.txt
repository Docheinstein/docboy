add_executable(docboy-sdl)

target_sources(docboy-sdl PRIVATE
        main.cpp
        window.h
        window.cpp
        glyphs.h
        glyphs.cpp
        window.cpp
)

target_link_libraries(docboy-sdl PRIVATE docboy utils extra args SDL3-static)

if (NOT WINDOWS)
    # SDL_Image with its dependencies it's a mess to build on Windows.
    # Disable it since we lose just the screenshot as a feature.
    target_compile_definitions(docboy-sdl PRIVATE SDL_IMAGE)
    target_link_libraries(docboy-sdl PRIVATE SDL3_image-static)
endif ()