set(SOURCES
        libretro.h
        libretro.cpp
)

add_library(docboy-libretro SHARED ${SOURCES})

set_target_properties(docboy-libretro PROPERTIES PREFIX "")
target_link_libraries(docboy-libretro PRIVATE core utils)