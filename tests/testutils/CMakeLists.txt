add_library(testutils STATIC)

target_sources(testutils PRIVATE
    img.h
    img.cpp
)

target_include_directories(testutils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

find_package(SDL2 REQUIRED)
target_include_directories(testutils PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(testutils PRIVATE ${SDL2_LIBRARY})

find_package(SDL2_IMAGE REQUIRED)
target_include_directories(testutils PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(testutils PRIVATE ${SDL2_IMAGE_LIBRARIES})

target_link_libraries(testutils PRIVATE utils)
