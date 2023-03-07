set(SOURCES
        main.cpp
        noguilcd.h
        noguilcd.cpp
)

add_executable(docboy-nogui ${SOURCES})

target_link_libraries(docboy-nogui PRIVATE core utils argumentum)