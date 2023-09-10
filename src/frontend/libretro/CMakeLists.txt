set(SOURCES
        libretro.h
        libretro.cpp
)

add_library(docboy_libretro SHARED ${SOURCES})

set_target_properties(docboy_libretro PROPERTIES PREFIX "")
target_link_libraries(docboy_libretro PRIVATE core utils)