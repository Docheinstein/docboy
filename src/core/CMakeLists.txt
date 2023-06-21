set(SOURCES
        boot/boot.cpp
        boot/boot.h
        boot/bootromfactory.cpp
        boot/bootromfactory.h
        boot/bootrom.h
        bus/bus.cpp
        bus/bus.h
        cartridge/cartridge.cpp
        cartridge/cartridgefactory.cpp
        cartridge/cartridgefactory.h
        cartridge/cartridge.h
        cartridge/mbc1.cpp
        cartridge/mbc1.h
        cartridge/mbc1ram.cpp
        cartridge/mbc1ram.h
        cartridge/nombc.cpp
        cartridge/nombc.h
        cartridge/slot.cpp
        cartridge/slot.h
        clock/clockable.h
        clock/clock.cpp
        clock/clock.h
        config/config.h
        config/config.cpp
        config/parser.h
        config/parser.cpp
        CMakeLists.txt
        core.cpp
        core.h
        cpu/cpu.cpp
        cpu/cpu.h
        cpu/interrupts.cpp
        cpu/interrupts.h
        cpu/timers.cpp
        cpu/timers.h
        dma/dma.cpp
        dma/dma.h
        definitions.h
        gameboy.cpp
        gameboy.h
        helpers.cpp
        helpers.h
        io/boot.h
        io/interrupts.h
        io/joypad.h
        io/lcd.h
        io/serial.h
        io/sound.h
        io/timers.h
        joypad/joypad.cpp
        joypad/joypad.h
        lcd/framebufferlcd.cpp
        lcd/framebufferlcd.h
        lcd/lcd.cpp
        lcd/lcd.h
        memory/memory.cpp
        memory/memory.h
        memory/readable.cpp
        memory/readable.h
        memory/writable.h
        ppu/lcdcontroller.cpp
        ppu/lcdcontroller.h
        ppu/ppu.cpp
        ppu/ppu.h
        serial/endpoint.h
        serial/endpoints/buffer.cpp
        serial/endpoints/buffer.h
        serial/endpoints/console.cpp
        serial/endpoints/console.h
        serial/link.cpp
        serial/link.h
        serial/port.cpp
        serial/port.h
        serial/serial.cpp
        serial/serial.h
        sound/sound.cpp
        sound/sound.h
)

add_library(core STATIC ${SOURCES})

if (ENABLE_DEBUGGER)
    target_sources(core PRIVATE
        debugger/backend.cpp
        debugger/backend.h
        debugger/boot/boot.cpp
        debugger/boot/boot.h
        debugger/cartridge/slot.cpp
        debugger/cartridge/slot.h
        debugger/core/core.cpp
        debugger/core/core.h
        debugger/cpu/cpu.cpp
        debugger/cpu/cpu.h
        debugger/cpu/interrupts.cpp
        debugger/cpu/interrupts.h
        debugger/cpu/timers.cpp
        debugger/cpu/timers.h
        debugger/frontendcli.cpp
        debugger/frontendcli.h
        debugger/frontend.h
        debugger/gameboy.cpp
        debugger/gameboy.h
        debugger/io/boot.h
        debugger/io/interrupts.h
        debugger/io/joypad.h
        debugger/io/lcd.h
        debugger/io/serial.h
        debugger/io/sound.h
        debugger/io/timers.h
        debugger/joypad/joypad.cpp
        debugger/joypad/joypad.h
        debugger/lcd/framebufferlcd.cpp
        debugger/lcd/framebufferlcd.h
        debugger/lcd/lcd.h
        debugger/memory/memory.cpp
        debugger/memory/memory.h
        debugger/memory/readable.cpp
        debugger/memory/readable.h
        debugger/ppu/lcdcontroller.cpp
        debugger/ppu/lcdcontroller.h
        debugger/ppu/ppu.cpp
        debugger/ppu/ppu.h
        debugger/serial/port.cpp
        debugger/serial/port.h
        debugger/shared.h
        debugger/sound/sound.cpp
        debugger/sound/sound.h
    )
    target_compile_definitions(core PUBLIC ENABLE_DEBUGGER)
endif()

if (ENABLE_PROFILER)
    target_sources(core PRIVATE
        profiler/profiler.cpp
        profiler/profiler.h
    )
    target_compile_definitions(core PUBLIC ENABLE_PROFILER)
endif ()

if (PROFILER_MAX_TICKS)
    target_compile_definitions(core PUBLIC PROFILER_MAX_TICKS=${PROFILER_MAX_TICKS})
endif ()

target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
