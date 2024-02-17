add_executable(docboy-nogui)

target_sources(docboy-nogui PRIVATE main.cpp)
target_link_libraries(docboy-nogui PRIVATE docboy utils extra args)