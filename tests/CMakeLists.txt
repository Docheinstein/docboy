include(CTest)

add_executable(docboy-tests)

add_test(NAME docboy-tests COMMAND docboy-tests)

target_sources(docboy-tests PRIVATE
        emutests.h
        main.cpp
        unittests.h
)

target_compile_definitions(docboy-tests PRIVATE "TESTS_ROOT_FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}\"")

target_link_libraries(docboy-tests PRIVATE docboy utils testutils extra Catch2::Catch2)

if (NOT ENABLE_SERIAL)
    message(FATAL_ERROR "Please compile with ENABLE_SERIAL=ON")
endif()

add_subdirectory(testutils)