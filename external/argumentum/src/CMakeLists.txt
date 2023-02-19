file(GLOB SOURCES "*.h" "*.cpp")

add_library(argumentum STATIC ${SOURCES})
target_include_directories(argumentum PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")