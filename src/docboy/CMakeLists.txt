add_library(docboy STATIC)

target_sources(docboy PRIVATE
        docboy/boot/boot.h
        docboy/boot/boot.cpp
        docboy/bus/bus.h
        docboy/bus/bus.cpp
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
        docboy/bus/wrambus.cpp
        docboy/bus/wrambus.h
        docboy/cartridge/cartridge.h
        docboy/cartridge/factory.cpp
        docboy/cartridge/factory.h
        docboy/cartridge/huc1/huc1.h
        docboy/cartridge/huc1/huc1.tpp
        docboy/cartridge/huc3/huc3.h
        docboy/cartridge/huc3/huc3.tpp
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
        docboy/cartridge/slot.cpp
        docboy/cartridge/slot.h
        docboy/common/macros.h
        docboy/common/randomdata.cpp
        docboy/common/randomdata.h
        docboy/common/specs.h
        docboy/core/core.cpp
        docboy/core/core.h
        docboy/cpu/cpu.cpp
        docboy/cpu/cpu.h
        docboy/cpu/idu.h
        docboy/dma/dma.cpp
        docboy/dma/dma.h
        docboy/gameboy/gameboy.h
        docboy/interrupts/interrupts.h
        docboy/joypad/joypad.cpp
        docboy/joypad/joypad.h
        docboy/lcd/colormap.h
        docboy/lcd/appearance.h
        docboy/lcd/lcd.cpp
        docboy/lcd/lcd.h
        docboy/memory/cell.h
        docboy/memory/fwd/cellfwd.h
        docboy/memory/fwd/hramfwd.h
        docboy/memory/fwd/memoryfwd.h
        docboy/memory/fwd/notusablefwd.h
        docboy/memory/fwd/oamfwd.h
        docboy/memory/fwd/vramfwd.h
        docboy/memory/fwd/wram1fwd.h
        docboy/memory/fwd/wram2fwd.h
        docboy/memory/hram.h
        docboy/memory/hram.cpp
        docboy/memory/memory.h
        docboy/memory/notusable.h
        docboy/memory/oam.h
        docboy/memory/traits.h
        docboy/memory/vram.h
        docboy/memory/vram0.cpp
        docboy/memory/vram0.h
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
        docboy/serial/endpoint.h
        docboy/serial/serial.cpp
        docboy/serial/serial.h
        docboy/audio/apu.cpp
        docboy/audio/apu.h
        docboy/timers/timers.cpp
        docboy/timers/timers.h
)

if (ENABLE_CGB)
    target_sources(docboy PRIVATE
            docboy/banks/vrambankcontroller.cpp
            docboy/banks/vrambankcontroller.h
            docboy/banks/wrambankcontroller.cpp
            docboy/banks/wrambankcontroller.h
            docboy/hdma/hdma.cpp
            docboy/hdma/hdma.h
            docboy/ir/infrared.cpp
            docboy/ir/infrared.h
            docboy/speedswitch/speedswitch.cpp
            docboy/speedswitch/speedswitch.h
            docboy/speedswitch/speedswitchcontroller.cpp
            docboy/speedswitch/speedswitchcontroller.h
            docboy/undoc/undocregs.cpp
            docboy/undoc/undocregs.h
    )
endif()

if (ENABLE_BOOTROM)
    target_compile_definitions(docboy PUBLIC ENABLE_BOOTROM)

    target_sources(docboy PRIVATE
            docboy/bootrom/bootrom.h
            docboy/bootrom/factory.cpp
            docboy/bootrom/factory.h
            docboy/bootrom/helpers.h
            docboy/bootrom/fwd/bootromfwd.h
    )
endif ()


if (ENABLE_DEBUGGER)
    target_compile_definitions(docboy PUBLIC ENABLE_DEBUGGER)

    if (NOT WIN32)
        # Fallback to ASCII characters for Windows
        target_compile_definitions(docboy PRIVATE TERM_SUPPORTS_UNICODE)
    endif()

    target_sources(docboy PRIVATE
            docboy/debugger/ansi.h
            docboy/debugger/backend.cpp
            docboy/debugger/backend.h
            docboy/debugger/frontend.cpp
            docboy/debugger/frontend.h
            docboy/debugger/helpers.cpp
            docboy/debugger/helpers.h
            docboy/debugger/memwatcher.cpp
            docboy/debugger/memwatcher.h
            docboy/debugger/mnemonics.cpp
            docboy/debugger/mnemonics.h
            docboy/debugger/shared.h
            docboy/debugger/symparser.h
            docboy/debugger/symparser.cpp
    )

    target_link_libraries(docboy PRIVATE tui)
endif ()

if (ENABLE_AUDIO)
    target_compile_definitions(docboy PUBLIC ENABLE_AUDIO)
endif ()

if (ENABLE_CGB)
    target_compile_definitions(docboy PUBLIC ENABLE_CGB)
endif ()

if (ENABLE_TWO_PLAYERS_MODE)
    target_compile_definitions(docboy PUBLIC ENABLE_TWO_PLAYERS_MODE)
endif ()

target_link_libraries(docboy PRIVATE utils)

target_include_directories(docboy PUBLIC .)
