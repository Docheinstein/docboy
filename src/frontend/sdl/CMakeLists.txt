set(SOURCES
        main.cpp
        window.h
        window.cpp
)

add_executable(docboy-sdl ${SOURCES})

# Find SDL2

find_package(SDL2 REQUIRED)
target_include_directories(docboy-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(docboy-sdl PRIVATE ${SDL2_LIBRARIES})

target_link_libraries(docboy-sdl PRIVATE core debugger log serial utils argumentum)