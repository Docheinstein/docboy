set(SOURCES
    arrayutils.h
    arrayutils.tpp
    binutils.h
    binutils.cpp
    binutils.tpp
    enumutils.h
    enumutils.tpp
    fileutils.h
    fileutils.cpp
    fileutils.tpp
    hexdump.h
    hexdump.cpp
    iniutils.h
    iniutils.cpp
    log.h
    log.cpp
    stdutils.h
    stdutils.tpp
    log.cpp
    strutils.h
    strutils.cpp
    termcolor.h
)

add_library(utils STATIC ${SOURCES})
target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
