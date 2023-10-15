add_library(docboy_libretro SHARED)

target_sources(docboy_libretro PRIVATE
    libretro.h
    libretro.cpp
)
target_link_libraries(docboy_libretro PRIVATE docboy utils)
