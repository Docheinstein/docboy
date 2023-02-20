set(SOURCES
    serial.h
    serial.cpp
    serialbuffer.h
    serialbuffer.cpp
    serialconsole.h
    serialconsole.cpp
)

add_library(serial STATIC ${SOURCES})
target_include_directories(serial PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
