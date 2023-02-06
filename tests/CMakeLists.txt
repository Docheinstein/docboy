set(SOURCES
        main.cpp
)

add_executable(gible-tests ${SOURCES})

target_link_libraries(gible-tests PRIVATE core utils Catch2::Catch2)