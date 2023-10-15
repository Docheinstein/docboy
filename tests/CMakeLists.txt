find_package(Catch2 REQUIRED)

add_executable(docboy-tests)

target_sources(docboy-tests PRIVATE main.cpp)
target_link_libraries(docboy-tests PRIVATE docboy utils Catch2::Catch2)

# Find SDL2 (optional: for tests roms)
find_package(SDL2)
find_package(SDL2_IMAGE)

if (SDL2_FOUND AND SDL2_IMAGE_FOUND)
    target_compile_definitions(docboy-tests PUBLIC SDL)
    target_include_directories(docboy-tests PRIVATE ${SDL2_INCLUDE_DIR})
    target_link_libraries(docboy-tests PRIVATE ${SDL2_LIBRARY})

    target_include_directories(docboy-tests PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(docboy-tests PRIVATE ${SDL2_IMAGE_LIBRARIES})
endif ()