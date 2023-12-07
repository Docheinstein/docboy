add_library(docboy STATIC)

target_sources(docboy PRIVATE
    boot/boot.h
    bootrom/bootrom.h
    bootrom/factory.cpp
    bootrom/factory.h
    bootrom/fwd/bootromfwd.h
    bootrom/macros.h
    bus/acqbus.h
    bus/bus.h
    bus/bus.hpp
    bus/cpubus.cpp
    bus/cpubus.h
    bus/extbus.cpp
    bus/extbus.h
    bus/fwd/busfwd.h
    bus/oambus.cpp
    bus/oambus.h
    bus/vrambus.cpp
    bus/vrambus.h
    cartridge/cartridge.h
    cartridge/factory.cpp
    cartridge/factory.h
    cartridge/helpers.h
    cartridge/mbc1/mbc1.h
    cartridge/mbc1/mbc1.tpp
    cartridge/mbc3/mbc3.h
    cartridge/mbc3/mbc3.tpp
    cartridge/mbc5/mbc5.h
    cartridge/mbc5/mbc5.tpp
    cartridge/nombc/nombc.h
    cartridge/nombc/nombc.tpp
    cartridge/slot.cpp
    cartridge/slot.h
    core/core.cpp
    core/core.h
    cpu/cpu.cpp
    cpu/cpu.h
    debugger/macros.h
    dma/dma.cpp
    dma/dma.h
    gameboy/gameboy.h
    interrupts/interrupts.h
    joypad/joypad.cpp
    joypad/joypad.h
    lcd/lcd.cpp
    lcd/lcd.h
    lcd/lcd.tpp
    memory/byte.cpp
    memory/byte.hpp
    memory/fwd/bytefwd.h
    memory/fwd/hramfwd.h
    memory/fwd/memoryfwd.h
    memory/fwd/oamfwd.h
    memory/fwd/vramfwd.h
    memory/fwd/wram1fwd.h
    memory/fwd/wram2fwd.h
    memory/hram.h
    memory/memory.hpp
    memory/oam.h
    memory/vram.h
    memory/wram1.h
    memory/wram2.h
    mmu/mmu.h
    mmu/mmu.hpp
    ppu/pixelmap.h
    ppu/ppu.cpp
    ppu/ppu.h
    ppu/video.h
    serial/endpoint.h
    serial/link.cpp
    serial/link.h
    serial/port.cpp
    serial/port.h
    serial/serial.h
    shared/specs.h
    sound/sound.h
    timers/timers.cpp
    timers/timers.h
)

if (ENABLE_BOOTROM)
    target_compile_definitions(docboy PUBLIC ENABLE_BOOTROM)

    target_sources(docboy PRIVATE
        boot/lock.h
        boot/lock.cpp
        bootrom/bootrom.h
        bootrom/factory.cpp
        bootrom/factory.h
        bootrom/fwd/bootromfwd.h
    )
endif ()


if (ENABLE_DEBUGGER)
    target_compile_definitions(docboy PUBLIC ENABLE_DEBUGGER)
    target_compile_definitions(docboy PUBLIC ENABLE_DEBUGGER_MEMORY_SNIFFER)

    target_sources(docboy PRIVATE
          debugger/backend.cpp
            debugger/backend.h
            debugger/frontend.cpp
            debugger/frontend.h
            debugger/helpers.cpp
            debugger/helpers.h
            debugger/memsniffer.cpp
            debugger/memsniffer.h
            debugger/mnemonics.cpp
            debugger/mnemonics.h
            debugger/shared.h
    )

    target_link_libraries(docboy PRIVATE tui)
endif ()

if (ENABLE_SERIAL)
    target_compile_definitions(docboy PUBLIC ENABLE_SERIAL)
endif ()

target_include_directories(docboy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
