set(SOURCES
        main.cpp
)

add_executable(docboy-tests ${SOURCES})

set_target_properties(docboy-tests PROPERTIES EXCLUDE_FROM_ALL True)

target_link_libraries(docboy-tests PRIVATE core utils Catch2::Catch2)