#ifndef GAMEBOY_H
#define GAMEBOY_H

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/bootrom.h"
#endif

#include <memory>

#include "docboy/boot/boot.h"
#include "docboy/boot/lock.h"
#include "docboy/bus/cpubus.h"
#include "docboy/bus/extbus.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "docboy/cartridge/slot.h"
#include "docboy/cpu/cpu.h"
#include "docboy/cpu/idu.hpp"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/lcd/lcd.h"
#include "docboy/memory/hram.h"
#include "docboy/memory/memory.hpp"
#include "docboy/memory/oam.h"
#include "docboy/memory/vram.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"
#include "docboy/mmu/mmu.h"
#include "docboy/ppu/ppu.h"
#include "docboy/ppu/video.h"
#include "docboy/serial/port.h"
#include "docboy/sound/sound.h"
#include "docboy/timers/timers.h"
#include "utils/macros.h"

class GameBoy {
public:
#ifdef ENABLE_BOOTROM
    explicit GameBoy(const Lcd::Palette& palette = Lcd::DEFAULT_PALETTE, std::unique_ptr<BootRom>&& bootRom = nullptr) :
        lcd {palette},
        bootRom {std::move(bootRom)} {
    }
#endif

    explicit GameBoy(const Lcd::Palette& palette = Lcd::DEFAULT_PALETTE) :
        lcd {palette} {
    }

    Vram vram {};
    Wram1 wram1 {};
    Wram2 wram2 {};
    Oam oam {};
    Hram hram {};

    IF_BOOTROM(std::unique_ptr<BootRom> bootRom {});
    CartridgeSlot cartridgeSlot {};
    BootIO boot {IF_BOOTROM(BootLock {mmu})};
    Joypad joypad {interrupts};
    SerialPort serialPort {interrupts};
    Timers timers {interrupts};
    InterruptsIO interrupts {};
    SoundIO sound {};
    VideoIO video {dma};
    ExtBus extBus {cartridgeSlot, wram1, wram2};
    CpuBus cpuBus {IF_BOOTROM(*bootRom COMMA) hram, joypad, serialPort, timers, interrupts, sound, video, boot};
    VramBus vramBus {vram};
    OamBus oamBus {oam};
    Mmu mmu {IF_BOOTROM(*bootRom COMMA) extBus, cpuBus, vramBus, oamBus};
    Dma dma {mmu, oamBus};
    Idu idu {oamBus};
    Cpu cpu {idu, interrupts, mmu};
    Lcd lcd {};
    Ppu ppu {lcd, video, interrupts, vramBus, oamBus};
};

#endif // GAMEBOY_H