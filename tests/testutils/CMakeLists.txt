add_library(testutils STATIC)

target_sources(testutils PRIVATE
    catch.h
    img.h
    img.cpp
    runners.hpp
)

target_include_directories(testutils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

find_package(SDL2 REQUIRED)
target_include_directories(testutils PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(testutils PRIVATE ${SDL2_LIBRARY} )

find_package(SDL2_IMAGE REQUIRED)
target_include_directories(testutils PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(testutils PRIVATE ${SDL2_IMAGE_LIBRARIES})

target_link_libraries(testutils PRIVATE docboy utils extra Catch2::Catch2)
