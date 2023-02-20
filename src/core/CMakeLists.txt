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
)

add_library(core STATIC ${SOURCES})


if (GAMEBOY_DOCTOR)
    target_compile_definitions(core PUBLIC GAMEBOY_DOCTOR)
endif()

target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
