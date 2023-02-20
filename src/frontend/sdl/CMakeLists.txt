set(SOURCES
        main.cpp
        window.h
        window.cpp
)

add_executable(gible-sdl ${SOURCES})

# Find SDL2

find_package(SDL2 REQUIRED)
target_include_directories(gible-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(gible-sdl PRIVATE ${SDL2_LIBRARIES})

target_link_libraries(gible-sdl PRIVATE core debugger log serial utils argumentum)