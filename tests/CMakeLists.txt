include(CTest)

add_executable(docboy-tests)

add_test(NAME docboy-emulation-tests COMMAND docboy-tests [emulation])
add_test(NAME docboy-unit-tests COMMAND docboy-tests [unit])

target_sources(docboy-tests PRIVATE
        main.cpp
        unittests.h
        runtests.h
        runtests.cpp
)

if (ENABLE_CGB)
    target_sources(docboy-tests PRIVATE
        cgbtests.h
    )
else ()
    target_sources(docboy-tests PRIVATE
        dmgtests.h
    )
endif()


target_compile_definitions(docboy-tests PRIVATE "TESTS_ROOT_FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}\"")

target_link_libraries(docboy-tests PRIVATE docboy utils testutils extra simdjson::simdjson Catch2::Catch2)

add_subdirectory(testutils)