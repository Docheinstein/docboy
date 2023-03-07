set(SOURCES
        main.cpp
        sdllcd.h
        sdllcd.cpp
        window.h
        window.cpp
)

add_executable(docboy-sdl ${SOURCES})

# Find SDL2
find_package(SDL2 REQUIRED)

target_include_directories(docboy-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(docboy-sdl PRIVATE core utils argumentum ${SDL2_LIBRARIES})