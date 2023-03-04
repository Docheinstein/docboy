#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <memory>
#include "cartridge/cartridge.h"
#include "definitions.h"
#include "bus.h"
#include "cpu/cpu.h"
#include "memory.h"
#include "vram.h"
#include "oam.h"
#include "io.h"
#include "gpu/gpu.h"

#ifdef ENABLE_DEBUGGER
#include "debugger/debuggablecpu.h"
#include "debugger/debuggablebus.h"
#endif

struct GameBoy {
    // TODO: typedef for all components?
    typedef Memory<MemoryMap::WRAM1::SIZE> WRAM1;
    typedef Memory<MemoryMap::WRAM2::SIZE> WRAM2;
    typedef Memory<MemoryMap::HRAM::SIZE> HRAM;
    typedef Memory<1> IE;

#ifdef ENABLE_DEBUGGER
    typedef IDebuggableCPU ICPU;
    typedef DebuggableCPU CPU;
#endif

    // TODO: std::unique_ptr<IMemory>?
    VRAM vram;
    WRAM1 wram1;
    WRAM2 wram2;
    OAM oam;
    IO io;
    HRAM hram;
    IE ie;

    std::unique_ptr<IBus> bus;
    std::unique_ptr<ICPU> cpu;

    std::unique_ptr<IDisplay> display;
    std::unique_ptr<IGPU> gpu;

    std::unique_ptr<Cartridge> cartridge;
};

#endif // GAMEBOY_H