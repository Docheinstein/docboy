add_library(testutils STATIC)

target_sources(testutils PRIVATE
    testutils/catch.h
    testutils/img.h
    testutils/img.cpp
    testutils/runners.hpp
)

target_link_libraries(testutils PRIVATE docboy utils extra Catch2::Catch2 SDL3-static SDL3_image-static)

target_include_directories(testutils PUBLIC .)
