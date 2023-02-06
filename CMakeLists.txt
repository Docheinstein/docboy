cmake_minimum_required(VERSION 3.24)
project(gible)

set(CMAKE_CXX_STANDARD 20)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(tests)

#
#file(GLOB_RECURSE sources "src/*.h" "src/*.cpp")
#list(FILTER sources EXCLUDE REGEX ".*/main.cpp$")
#
#add_executable(gible ${sources} src/main.cpp)
#add_executable(gible-qt ${sources} src/main.cpp)
#add_executable(gible-tests ${sources} tests/main.cpp)
#
## src
#target_include_directories(gible PRIVATE src)
#target_include_directories(gible-qt PRIVATE src)
#target_include_directories(gible-tests PRIVATE src)
#
#
## SDL2
#find_package(SDL2 REQUIRED)
#target_include_directories(gible PRIVATE ${SDL2_INCLUDE_DIRS})
#target_link_libraries(gible PRIVATE ${SDL2_LIBRARIES})
#
## QT
#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGLWidgets)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets)
#target_link_libraries(gible PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets)
#
## Catch2
#add_subdirectory(external/catch2)
#target_link_libraries(gible-tests PRIVATE Catch2::Catch2)
#
## TODO: not needed
#target_link_libraries(gible-tests PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets)
