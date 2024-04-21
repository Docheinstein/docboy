add_library(docboy STATIC)

target_sources(docboy PRIVATE
        docboy/boot/boot.h
        docboy/bootrom/macros.h
        docboy/bus/bus.h
        docboy/bus/bus.tpp
        docboy/bus/cpubus.cpp
        docboy/bus/cpubus.h
        docboy/bus/device.h
        docboy/bus/extbus.cpp
        docboy/bus/extbus.h
        docboy/bus/fwd/busfwd.h
        docboy/bus/oambus.cpp
        docboy/bus/oambus.h
        docboy/bus/oambus.tpp
        docboy/bus/videobus.h
        docboy/bus/videobus.tpp
        docboy/bus/vrambus.cpp
        docboy/bus/vrambus.h
        docboy/bus/vrambus.tpp
        docboy/cartridge/cartridge.h
        docboy/cartridge/factory.cpp
        docboy/cartridge/factory.h
        docboy/cartridge/helpers.h
        docboy/cartridge/mbc1/mbc1.h
        docboy/cartridge/mbc1/mbc1.tpp
        docboy/cartridge/mbc1/mbc1m.h
        docboy/cartridge/mbc2/mbc2.h
        docboy/cartridge/mbc2/mbc2.tpp
        docboy/cartridge/mbc3/mbc3.h
        docboy/cartridge/mbc3/mbc3.tpp
        docboy/cartridge/mbc5/mbc5.h
        docboy/cartridge/mbc5/mbc5.tpp
        docboy/cartridge/nombc/nombc.h
        docboy/cartridge/nombc/nombc.tpp
        docboy/cartridge/romram/romram.h
        docboy/cartridge/romram/romram.tpp
        docboy/cartridge/rtc/rtc.h
        docboy/cartridge/slot.cpp
        docboy/cartridge/slot.h
        docboy/core/core.cpp
        docboy/core/core.h
        docboy/cpu/cpu.cpp
        docboy/cpu/cpu.h
        docboy/cpu/idu.hpp
        docboy/dma/dma.cpp
        docboy/dma/dma.h
        docboy/gameboy/gameboy.h
        docboy/interrupts/interrupts.h
        docboy/joypad/joypad.cpp
        docboy/joypad/joypad.h
        docboy/lcd/lcd.cpp
        docboy/lcd/lcd.h
        docboy/memory/byte.cpp
        docboy/memory/byte.hpp
        docboy/memory/fwd/bytefwd.h
        docboy/memory/fwd/hramfwd.h
        docboy/memory/fwd/memoryfwd.h
        docboy/memory/fwd/oamfwd.h
        docboy/memory/fwd/randmemoryfwd.h
        docboy/memory/fwd/vramfwd.h
        docboy/memory/fwd/wram1fwd.h
        docboy/memory/fwd/wram2fwd.h
        docboy/memory/hram.h
        docboy/memory/memory.hpp
        docboy/memory/oam.h
        docboy/memory/randmemory.hpp
        docboy/memory/vram.cpp
        docboy/memory/vram.h
        docboy/memory/wram1.h
        docboy/memory/wram2.h
        docboy/mmu/mmu.cpp
        docboy/mmu/mmu.h
        docboy/mmu/mmu.tpp
        docboy/stop/stopcontroller.cpp
        docboy/stop/stopcontroller.h
        docboy/ppu/pixelmap.h
        docboy/ppu/ppu.cpp
        docboy/ppu/ppu.h
        docboy/ppu/video.h
        docboy/serial/endpoint.h
        docboy/serial/link.cpp
        docboy/serial/link.h
        docboy/serial/port.cpp
        docboy/serial/port.h
        docboy/serial/serial.h
        docboy/shared/macros.h
        docboy/shared/specs.h
        docboy/sound/sound.h
        docboy/timers/timers.cpp
        docboy/timers/timers.h
)

if (ENABLE_BOOTROM)
    target_compile_definitions(docboy PUBLIC ENABLE_BOOTROM)

    target_sources(docboy PRIVATE
            docboy/boot/lock.h
            docboy/boot/lock.cpp
            docboy/bootrom/bootrom.h
            docboy/bootrom/factory.cpp
            docboy/bootrom/factory.h
            docboy/bootrom/fwd/bootromfwd.h
    )
endif ()


if (ENABLE_DEBUGGER)
    target_compile_definitions(docboy PUBLIC ENABLE_DEBUGGER)
    target_compile_definitions(docboy PUBLIC ENABLE_DEBUGGER_MEMORY_SNIFFER)

    if (NOT WIN32)
        # Fallback to ASCII characters for Windows
        target_compile_definitions(docboy PRIVATE TERM_SUPPORTS_UNICODE)
    endif()

    target_sources(docboy PRIVATE
            docboy/debugger/backend.cpp
            docboy/debugger/backend.h
            docboy/debugger/frontend.cpp
            docboy/debugger/frontend.h
            docboy/debugger/helpers.cpp
            docboy/debugger/helpers.h
            docboy/debugger/memsniffer.cpp
            docboy/debugger/memsniffer.h
            docboy/debugger/mnemonics.cpp
            docboy/debugger/mnemonics.h
            docboy/debugger/shared.h
    )

    target_link_libraries(docboy PRIVATE tui)
endif ()

if (ENABLE_SERIAL)
    target_compile_definitions(docboy PUBLIC ENABLE_SERIAL)
endif ()

target_link_libraries(docboy PRIVATE utils)

target_include_directories(docboy PUBLIC .)
