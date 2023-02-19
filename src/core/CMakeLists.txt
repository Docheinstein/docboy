set(SOURCES
    bus.h
    bus.cpp
    cartridge.h
    cartridge.cpp
    cpu.h
    cpu.cpp
    definitions.h
    gible.h
    gible.cpp
    io.h
    io.cpp
    memory.h
    memory.cpp
    serial.h
    serial.cpp
    serialconsole.h
    serialconsole.cpp
)

add_library(core STATIC ${SOURCES})
target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
