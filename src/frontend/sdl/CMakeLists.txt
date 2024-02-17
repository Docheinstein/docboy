add_executable(docboy-sdl)

target_sources(docboy-sdl PRIVATE
        main.cpp
        window.h
        window.cpp
        glyphs.h
        glyphs.cpp
        window.cpp
)


# SDL2
find_package(SDL2 REQUIRED)

target_include_directories(docboy-sdl PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(docboy-sdl PRIVATE docboy utils extra args ${SDL2_LIBRARY})

# SDL2_Image
find_package(SDL2_IMAGE)

if (SDL2_IMAGE_FOUND)
    target_compile_definitions(docboy-sdl PRIVATE SDL2_IMAGE)
    target_include_directories(docboy-sdl PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(docboy-sdl PRIVATE ${SDL2_IMAGE_LIBRARIES})
endif()
