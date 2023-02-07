set(SOURCES
        main.cpp
)

add_executable(gible-tests ${SOURCES})
target_compile_definitions(gible-tests PUBLIC TESTING=1)

target_link_libraries(gible-tests PRIVATE core log utils Catch2::Catch2)