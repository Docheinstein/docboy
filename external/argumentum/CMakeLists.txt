add_library(argumentum STATIC EXCLUDE_FROM_ALL)

file(GLOB SOURCES "*.h" "*.cpp")
target_sources(argumentum PRIVATE ${SOURCES})
target_include_directories(argumentum PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_compile_options(argumentum PRIVATE -w) # not interested in warnings

