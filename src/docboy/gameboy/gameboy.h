#ifndef GAMEBOY_H
#define GAMEBOY_H

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/bootrom.h"
#endif

#include "docboy/boot/boot.h"
#include "docboy/bus/bus.h"
#include "docboy/cartridge/slot.h"
#include "docboy/cpu/cpu.h"
#include "docboy/cpu/interrupts.h"
#include "docboy/cpu/timers.h"
#include "docboy/joypad/joypad.h"
#include "docboy/lcd/lcd.h"
#include "docboy/memory/hram.h"
#include "docboy/memory/memory.hpp"
#include "docboy/memory/null.h"
#include "docboy/memory/oam.h"
#include "docboy/memory/vram.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"
#include "docboy/ppu/ppu.h"
#include "docboy/ppu/video.h"
#include "docboy/serial/port.h"
#include "docboy/sound/sound.h"
#include "utils/macros.h"

class GameBoy {
public:
#ifdef ENABLE_BOOTROM
    explicit GameBoy(std::unique_ptr<BootRom>&& bootRom = nullptr) :
        bootRom(std::move(bootRom)) {
    }
#endif

    Vram vram {};
    Wram1 wram1 {};
    Wram2 wram2 {};
    Oam oam {};
    Hram hram {};

    Dma dma {bus, oam};
    IF_BOOTROM(std::unique_ptr<BootRom> bootRom {});
    CartridgeSlot cartridgeSlot {};
    BootIO boot {};
    Joypad joypad {interrupts};
    SerialPort serialPort {interrupts};
    Timers timers {interrupts};
    InterruptsIO interrupts {};
    SoundIO sound {};
    VideoIO video {dma};
    NullIO nullIO {};
    Bus bus {IF_BOOTROM(*bootRom COMMA) cartridgeSlot,
             vram,
             wram1,
             wram2,
             oam,
             hram,
             joypad,
             serialPort,
             timers,
             interrupts,
             sound,
             video,
             boot,
             nullIO};
    Cpu cpu {interrupts, timers, serialPort, boot, bus};
    Lcd lcd {};
    Ppu ppu {lcd, video, interrupts, vram, oam};
};

#endif // GAMEBOY_H