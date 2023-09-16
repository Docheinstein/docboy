set(SOURCES
    arrayutils.h
    arrayutils.tpp
    binutils.cpp
    binutils.h
    binutils.tpp
    enumutils.h
    enumutils.tpp
    exceptionutils.h
    hexdump.cpp
    hexdump.h
    iniutils.cpp
    iniutils.h
    ioutils.cpp
    ioutils.h
    ioutils.tpp
    log.cpp
    log.h
    osutils.cpp
    osutils.h
    path.cpp
    path.h
    stdutils.h
    stdutils.tpp
    strutils.cpp
    strutils.h
    termcolor.h
    timeutils.cpp
    timeutils.h
)

add_library(utils STATIC ${SOURCES})

target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
