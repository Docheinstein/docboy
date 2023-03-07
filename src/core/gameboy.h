#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <memory>
#include "components.h"
#include "cartridge/cartridge.h"
#include "definitions.h"
#include "bus.h"
#include "cpu/cpu.h"
#include "memory.h"
#include "vram.h"
#include "oam.h"
#include "io.h"
#include "ppu/lcd.h"
#include "ppu/ppu.h"

struct GameBoy {
    GameBoy();

    VRAM vram;
    MemoryImpl wram1;
    MemoryImpl wram2;
    OAM oam;
    IO io;
    MemoryImpl hram;
    MemoryImpl ie;

    std::unique_ptr<IBus> bus;
    std::unique_ptr<ICPUImpl> cpu;

    std::unique_ptr<ILCDImpl> lcd;
    std::unique_ptr<IPPUImpl> ppu;

    std::unique_ptr<Cartridge> cartridge;
};

#endif // GAMEBOY_H