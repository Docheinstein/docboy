set(SOURCES
    bus.h
    bus.cpp
    cartridge.h
    cartridge.cpp
    cpu.h
    cpu.cpp
    gible.h
    gible.cpp
    memory.h
    memory.cpp
)

add_library(core STATIC ${SOURCES})
target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
