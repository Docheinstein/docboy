set(SOURCES
        fileutils.h
        fileutils.cpp
        binutils.h
        binutils.tpp
        binutils.cpp
)

add_library(utils STATIC ${SOURCES})
target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
