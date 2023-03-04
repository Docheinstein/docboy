set(SOURCES
    binutils.h
    binutils.tpp
    binutils.cpp
    fileutils.h
    fileutils.cpp
    hexdump.h
    hexdump.cpp
    log.h
    log.cpp
    strutils.h
    strutils.cpp
    termcolor.h
)

add_library(utils STATIC ${SOURCES})
target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
