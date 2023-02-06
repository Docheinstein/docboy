set(SOURCES
        log.h
        log.cpp
)

add_library(log STATIC ${SOURCES})
target_include_directories(log PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
