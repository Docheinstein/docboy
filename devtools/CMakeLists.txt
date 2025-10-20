add_executable(cmpframebuffers cmpframebuffers.cpp)
target_link_libraries(cmpframebuffers PRIVATE testutils utils)

add_executable(convframebuffer convframebuffer.cpp)
target_link_libraries(convframebuffer PRIVATE testutils utils)

add_executable(dumpframebuffer dumpframebuffer.cpp)
target_link_libraries(dumpframebuffer PRIVATE testutils)

add_executable(paletteframebuffer paletteframebuffer.cpp)
target_link_libraries(paletteframebuffer PRIVATE testutils utils)

add_executable(rgb565to888 rgb565to888.cpp)
target_link_libraries(rgb565to888 PRIVATE testutils utils extra)

add_executable(rgb888to565 rgb888to565.cpp)
target_link_libraries(rgb888to565 PRIVATE testutils utils extra)

add_executable(runcmpframebuffer runcmpframebuffer.cpp)
target_link_libraries(runcmpframebuffer PRIVATE testutils docboy utils extra)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE testutils docboy utils args)

add_executable(dumpstate dumpstate.cpp)
target_link_libraries(dumpstate PRIVATE testutils docboy utils)
