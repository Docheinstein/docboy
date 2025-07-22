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
#include "docboy/serial/serial.h"
#include "docboy/stop/stopcontroller.h"
#include "docboy/timers/timers.h"

#ifdef ENABLE_CGB
#include "docboy/banks/vrambankcontroller.h"
#include "docboy/banks/wrambankcontroller.h"
#include "docboy/bus/wrambus.h"
#include "docboy/hdma/hdma.h"
#include "docboy/ir/infrared.h"
#include "docboy/memory/notusable.h"
#include "docboy/speedswitch/speedswitch.h"
#include "docboy/speedswitch/speedswitchcontroller.h"
#include "docboy/undoc/undocregs.h"
#endif

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/bootrom.h"
#endif

class GameBoy {
public:
#ifdef ENABLE_CGB
    Cpu cpu {idu, interrupts, mmu, joypad, stop_controller, speed_switch, speed_switch_controller};
#else
    Cpu cpu {idu, interrupts, mmu, joypad, stop_controller};
#endif
    Idu idu {oam_bus};

    // Video
#ifdef ENABLE_CGB
    Ppu ppu {lcd, interrupts, hdma, vram_bus, oam_bus, dma, speed_switch_controller};
#else
    Ppu ppu {lcd, interrupts, vram_bus, oam_bus, dma};
#endif
    Lcd lcd {};

    // DMA
    Dma dma {mmu, oam_bus};

    // Audio
#ifdef ENABLE_CGB
    Apu apu {timers, speed_switch_controller, cpu.pc};
#else
    Apu apu {timers, cpu.pc};
#endif

    // Power Saving
    StopController stop_controller {joypad, timers, ppu, lcd};

    // Boot ROM
#ifdef ENABLE_BOOTROM
    BootRom boot_rom {};
#endif

    // Cartridge
    CartridgeSlot cartridge_slot {};

    // Memory
#ifdef ENABLE_CGB
    Vram vram[2] {};
#else
    Vram vram[1] {};
#endif

    Wram1 wram1 {};

#ifdef ENABLE_CGB
    Wram2 wram2[7] {};
#else
    Wram2 wram2[1] {};
#endif

    Oam oam {};

#ifdef ENABLE_CGB
    NotUsable not_usable {};
#endif

    Hram hram {};

    // IO
#ifdef ENABLE_BOOTROM
    Boot boot {mmu};
#else
    Boot boot {};
#endif
    Joypad joypad {interrupts};
    Serial serial {timers, interrupts};
    Timers timers {interrupts};
    Interrupts interrupts {};

    // Buses
#ifdef ENABLE_CGB
    ExtBus ext_bus {cartridge_slot};
#else
    ExtBus ext_bus {cartridge_slot, wram1, wram2};
#endif

#ifdef ENABLE_CGB
    WramBus wram_bus {wram1, wram2};
#endif

#ifdef ENABLE_BOOTROM
#ifdef ENABLE_CGB
    CpuBus cpu_bus {
        boot_rom,
        hram,
        joypad,
        serial,
        timers,
        interrupts,
        boot,
        apu,
        ppu,
        dma,
        vram_bank_controller,
        wram_bank_controller,
        hdma,
        speed_switch,
        infrared,
        undocumented_registers,
    };
#else
    CpuBus cpu_bus {boot_rom, hram, joypad, serial, timers, interrupts, boot, apu, ppu, dma};
#endif
#else
#ifdef ENABLE_CGB
    CpuBus cpu_bus {hram,
                    joypad,
                    serial,
                    timers,
                    interrupts,
                    boot,
                    apu,
                    ppu,
                    dma,
                    vram_bank_controller,
                    wram_bank_controller,
                    hdma,
                    speed_switch,
                    infrared,
                    undocumented_registers};
#else
    CpuBus cpu_bus {hram, joypad, serial, timers, interrupts, boot, apu, ppu, dma};
#endif
#endif
    VramBus vram_bus {vram};

#ifdef ENABLE_CGB
    OamBus oam_bus {oam, not_usable};
#else
    OamBus oam_bus {oam};
#endif

    // MMU
#if ENABLE_BOOTROM
#ifdef ENABLE_CGB
    Mmu mmu {boot_rom, ext_bus, wram_bus, cpu_bus, vram_bus, oam_bus};
#else
    Mmu mmu {boot_rom, ext_bus, cpu_bus, vram_bus, oam_bus};
#endif
#else
#ifdef ENABLE_CGB
    Mmu mmu {ext_bus, wram_bus, cpu_bus, vram_bus, oam_bus};
#else
    Mmu mmu {ext_bus, cpu_bus, vram_bus, oam_bus};
#endif
#endif

#ifdef ENABLE_CGB
    // Bank Controllers
    VramBankController vram_bank_controller {vram_bus};
    WramBankController wram_bank_controller {wram_bus};

    // HDMA
    Hdma hdma {mmu,
               ext_bus,
               vram_bus,
               oam_bus,
               ppu.stat.mode,
               cpu.fetching,
               cpu.halted,
               stop_controller.stopped,
               speed_switch_controller};

#ifdef ENABLE_CGB
    // Speed Switch
    SpeedSwitch speed_switch {};
    SpeedSwitchController speed_switch_controller {speed_switch, interrupts, timers, cpu.halted};
#endif

    // Other CGB components
    Infrared infrared {};
    UndocumentedRegisters undocumented_registers {};
#endif
};

#endif // GAMEBOY_H