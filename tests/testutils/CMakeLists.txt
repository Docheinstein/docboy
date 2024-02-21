add_library(testutils STATIC)

target_sources(testutils PRIVATE
    testutils/catch.h
    testutils/framebuffers.h
    testutils/framebuffers.cpp
    testutils/runners.hpp
)

target_link_libraries(testutils PRIVATE docboy utils extra Catch2::Catch2)

target_include_directories(testutils PUBLIC .)
