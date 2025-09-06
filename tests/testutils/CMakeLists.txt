add_library(testutils STATIC)

target_sources(testutils PRIVATE
    testutils/framebuffers.h
    testutils/framebuffers.cpp
    testutils/runners.h
)

target_link_libraries(testutils PRIVATE docboy utils extra)

target_include_directories(testutils PUBLIC .)
