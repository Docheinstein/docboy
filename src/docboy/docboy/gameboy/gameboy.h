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
#include "docboy/memory/vram0.h"
#include "docboy/memory/vram1.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"
#include "docboy/mmu/mmu.h"
#include "docboy/ppu/ppu.h"
#include "docboy/serial/port.h"
#include "docboy/stop/stopcontroller.h"
#include "docboy/timers/timers.h"

#ifdef ENABLE_CGB
#include "docboy/bankswitch/vrambankswitch.h"
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
    Vram0 vram0 {};

#ifdef ENABLE_CGB
    Vram1 vram1 {};
#endif

    Wram1 wram1 {};
    Wram2 wram2 {};
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
    CpuBus cpu_bus {*boot_rom, hram, joypad, serial_port, timers, interrupts, apu, ppu, vram_bank_switch, boot};
#else
    CpuBus cpu_bus {*boot_rom, hram, joypad, serial_port, timers, interrupts, apu, ppu, boot};
#endif
#else
#ifdef ENABLE_CGB
    CpuBus cpu_bus {hram, joypad, serial_port, timers, interrupts, apu, ppu, vram_bank_switch, boot};
#else
    CpuBus cpu_bus {hram, joypad, serial_port, timers, interrupts, apu, ppu, boot};
#endif
#endif
#ifdef ENABLE_CGB
    VramBus vram_bus {vram0, vram1};
#else
    VramBus vram_bus {vram0};
#endif
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
    Ppu ppu {lcd, interrupts, dma, vram_bus, oam_bus};

    // Audio
    Apu apu {timers};

    // Power Saving
    bool stopped {};
    StopController stop_controller {stopped, joypad, timers, lcd};

#ifdef ENABLE_CGB
    VramBankSwitch vram_bank_switch {vram_bus};
#endif
};

#endif // GAMEBOY_H