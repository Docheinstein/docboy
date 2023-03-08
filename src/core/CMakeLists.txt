set(SOURCES
        boot/bootrom.h
        boot/bootromfactory.h
        boot/bootromfactory.cpp
        bus/bus.h
        bus/bus.cpp
        cartridge/cartridge.h
        cartridge/cartridge.cpp
        cartridge/cartridgefactory.h
        cartridge/cartridgefactory.cpp
        clock/clock.h
        clock/clock.cpp
        clock/clockable.h
        impl/bootrom.h
        impl/cpu.h
        impl/lcd.h
        impl/memory.h
        impl/ppu.h
        cpu/cpu.h
        cpu/cpu.cpp
        memory/io.h
        memory/io.cpp
        memory/memory.h
        memory/memory.cpp
        memory/readable.h
        memory/readable.cpp
        memory/writable.h
        cartridge/mbc1.h
        cartridge/mbc1.cpp
        cartridge/mbc1ram.h
        cartridge/mbc1ram.cpp
        cartridge/nombc.h
        cartridge/nombc.cpp
        ppu/lcd.h
        ppu/lcd.cpp
        ppu/ppu.h
        ppu/ppu.cpp
        serial/endpoints/buffer.h
        serial/endpoints/buffer.cpp
        serial/endpoints/console.h
        serial/endpoints/console.cpp
        serial/endpoint.h
        serial/link.h
        serial/link.cpp
        serial/port.h
        serial/port.cpp
        core.h
        core.cpp
        definitions.h
        gameboy.h
        gameboy.cpp
        helpers.h
        helpers.cpp
)

add_library(core STATIC ${SOURCES})

if (ENABLE_DEBUGGER)
    target_sources(core PRIVATE
            debugger/backend.h
            debugger/backend.cpp
            debugger/frontend.h
            debugger/frontend.cpp
            debugger/frontendcli.h
            debugger/frontendcli.cpp
            debugger/boot/bootrom.h
            debugger/core/core.h
            debugger/core/core.cpp
            debugger/cpu/cpu.h
            debugger/cpu/cpu.cpp
            debugger/ppu/lcd.h
            debugger/ppu/lcd.cpp
            debugger/memory/memory.h
            debugger/memory/memory.cpp
            debugger/memory/readable.h
            debugger/memory/readable.cpp
            debugger/ppu/ppu.h
            debugger/ppu/ppu.cpp
    )
    target_compile_definitions(core PUBLIC ENABLE_DEBUGGER)
endif()

target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
