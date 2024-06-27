#ifndef CPUBUS_H
#define CPUBUS_H

#include "docboy/bus/bus.h"
#include "docboy/memory/fwd/hramfwd.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/fwd/bootromfwd.h"
#endif

class Joypad;
class Serial;
class Timers;
class Interrupts;
class Apu;
class Ppu;
class Boot;

class CpuBus final : public Bus {

public:
#ifdef ENABLE_BOOTROM
    < < < < < < < HEAD CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers,
                              Interrupts& interrupts, Apu& apu, Ppu& ppu, Boot& boot);
#else
    CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Apu& apu, Ppu& Ppu,
           Boot& boot);
#endif

#ifdef ENABLE_BOOTROM
    BootRom& boot_rom;
#endif

    Hram& hram;

    Joypad& joypad;
    Serial& serial;
    Timers& timers;
    Interrupts& interrupts;
    Boot& boot;

    Apu& apu;
    Ppu& ppu;
};
#endif // CPUBUS_H