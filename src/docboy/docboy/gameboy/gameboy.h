#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <memory>

#include "docboy/audio/apu.h"
#include "docboy/boot/boot.h"
#include "docboy/bus/cpubus.h"
#include "docboy/bus/extbus.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "docboy/cartridge/slot.h"
#include "docboy/cpu/cpu.h"
#include "docboy/cpu/idu.h"
#include "docboy/dma/dma.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/lcd/lcd.h"
#include "docboy/memory/hram.h"
#include "docboy/memory/memory.h"
#include "docboy/memory/oam.h"
#include "docboy/memory/vram.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"
#include "docboy/mmu/mmu.h"
#include "docboy/ppu/ppu.h"
#include "docboy/serial/port.h"
#include "docboy/stop/stopcontroller.h"
#include "docboy/timers/timers.h"

#ifdef ENABLE_CGB
#include "docboy/banks/vrambankcontroller.h"
#include "docboy/banks/wrambankcontroller.h"
#include "docboy/hdma/hdma.h"
#include "docboy/ir/infrared.h"
#include "docboy/undoc/undocregs.h"
#endif

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/bootrom.h"
#endif

class GameBoy {
public:
#ifdef ENABLE_BOOTROM
    explicit GameBoy(std::unique_ptr<BootRom> boot_rom) :
        boot_rom {std::move(boot_rom)} {
    }
#endif

    // Memory
#ifdef ENABLE_CGB
    Vram vram[2] {};
#else
    Vram vram[1] {};
#endif

    Wram1 wram1 {};
#ifdef ENABLE_CGB
    Wram2 wram2[8] {};
#else
    Wram2 wram2[1] {};
#endif
    Oam oam {};
    Hram hram {};

    // Boot ROM
#ifdef ENABLE_BOOTROM
    std::unique_ptr<BootRom> boot_rom {};
#endif

    // Cartridge
    CartridgeSlot cartridge_slot {};

    // IO
#ifdef ENABLE_BOOTROM
    Boot boot {mmu};
#else
    Boot boot {};
#endif
    Joypad joypad {interrupts};
    SerialPort serial_port {interrupts};
    Timers timers {interrupts};
    Interrupts interrupts {};

    // Buses
    ExtBus ext_bus {cartridge_slot, wram1, wram2};
#ifdef ENABLE_BOOTROM
#ifdef ENABLE_CGB
    CpuBus cpu_bus {
        *boot_rom,
        hram,
        joypad,
        serial_port,
        timers,
        interrupts,
        boot,
        apu,
        ppu,
        dma,
        hdma,
        vram_bank_controller,
        wram_bank_controller,
        infrared,
        undocumented_registers,
    };
#else
    CpuBus cpu_bus {*boot_rom, hram, joypad, serial_port, timers, interrupts, boot, apu, ppu};
#endif
#else
#ifdef ENABLE_CGB
    CpuBus cpu_bus {hram,
                    joypad,
                    serial_port,
                    timers,
                    interrupts,
                    boot,
                    apu,
                    ppu,
                    dma,
                    hdma,
                    vram_bank_controller,
                    wram_bank_controller,
                    infrared,
                    undocumented_registers};
#else
    CpuBus cpu_bus {hram, joypad, serial_port, timers, interrupts, boot, apu, ppu, dma};
#endif
#endif
    VramBus vram_bus {vram};
    OamBus oam_bus {oam};

    // MMU
#if ENABLE_BOOTROM
    Mmu mmu {*boot_rom, ext_bus, cpu_bus, vram_bus, oam_bus};
#else
    Mmu mmu {ext_bus, cpu_bus, vram_bus, oam_bus};
#endif

    // DMA
    Dma dma {mmu, oam_bus};

    // CPU
    Idu idu {oam_bus};
    Cpu cpu {idu, interrupts, mmu, joypad, stop_controller};

    // Video
    Lcd lcd {};
#ifdef ENABLE_CGB
    Ppu ppu {lcd, interrupts, hdma, vram_bus, oam_bus};
#else
    Ppu ppu {lcd, interrupts, vram_bus, oam_bus};
#endif

    // Audio
    Apu apu {timers};

    // Power Saving
    bool stopped {};
    StopController stop_controller {stopped, joypad, timers, lcd};

#ifdef ENABLE_CGB
    VramBankController vram_bank_controller {vram_bus};
    WramBankController wram_bank_controller {ext_bus};

    Hdma hdma {mmu, vram_bus};
    Infrared infrared {};
    UndocumentedRegisters undocumented_registers {};
#endif
};

#endif // GAMEBOY_H