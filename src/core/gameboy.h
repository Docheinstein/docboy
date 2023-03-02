#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <memory>
#include "cartridge/cartridge.h"
#include "bus.h"
#include "cpu.h"
#include "memory.h"
#include "io.h"

#ifdef ENABLE_DEBUGGER
#include "debugger/debuggablecpu.h"
#include "debugger/debuggablebus.h"
#endif

struct GameBoy {
    GameBoy();

#ifdef ENABLE_DEBUGGER
    DebuggableBus bus;
    DebuggableCpu cpu;
#else
    Bus bus;
    Cpu cpu;
#endif

    Memory<4096> wram1;
    Memory<4096> wram2;
    IO io;
    Memory<127> hram;
    Memory<1> ie;

    std::unique_ptr<Cartridge> cartridge;
};

#endif // GAMEBOY_H