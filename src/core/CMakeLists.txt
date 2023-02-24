set(SOURCES
    bus.h
    bus.cpp
    cartridge.h
    cartridge.cpp
    cartridgefactory.h
    cartridgefactory.cpp
    cpu.h
    cpu.cpp
    instructions.h
    io.h
    io.cpp
    mbc1.h
    mbc1.cpp
    mbc1ram.h
    mbc1ram.cpp
    gible.h
    gible.cpp
    memory.h
    memory.cpp
    memorymap.h
    nombc.h
    nombc.cpp
)

add_library(core STATIC ${SOURCES})


if (GAMEBOY_DOCTOR)
    target_compile_definitions(core PUBLIC GAMEBOY_DOCTOR)
endif()

target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
