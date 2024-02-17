find_package(Catch2 REQUIRED)

add_executable(docboy-tests)

target_sources(docboy-tests PRIVATE
        emutests.hpp
        main.cpp
        unittests.hpp
)

target_link_libraries(docboy-tests PRIVATE docboy utils testutils extra Catch2::Catch2 SDL3-static SDL3_image-static)

if (NOT ENABLE_SERIAL)
    message(FATAL_ERROR "Please compile with ENABLE_SERIAL=ON")
endif()

add_subdirectory(testutils)