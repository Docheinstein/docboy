set(SOURCES
        main.cpp
)

add_executable(docboy-nogui ${SOURCES})

target_link_libraries(docboy-nogui PRIVATE core utils argumentum)