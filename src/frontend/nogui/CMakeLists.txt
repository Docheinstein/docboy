set(SOURCES
        main.cpp
)

add_executable(gible-nogui ${SOURCES})

target_link_libraries(gible-nogui PRIVATE core debugger log utils argumentum)