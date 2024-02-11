find_package(Catch2 REQUIRED)

add_executable(docboy-tests)

target_sources(docboy-tests PRIVATE
        emutests.hpp
        main.cpp
        unittests.hpp
)

target_link_libraries(docboy-tests PRIVATE docboy utils testutils extra Catch2::Catch2)

find_package(SDL2 REQUIRED)
target_compile_definitions(docboy-tests PUBLIC SDL)
target_include_directories(docboy-tests PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(docboy-tests PRIVATE ${SDL2_LIBRARY})

find_package(SDL2_IMAGE REQUIRED)
target_include_directories(docboy-tests PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(docboy-tests PRIVATE ${SDL2_IMAGE_LIBRARIES})

if (NOT ENABLE_SERIAL)
    message(FATAL_ERROR "Please compile with ENABLE_SERIAL=ON")
endif()

add_subdirectory(testutils)