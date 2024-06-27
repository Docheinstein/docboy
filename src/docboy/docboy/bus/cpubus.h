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
class Sound;
class Ppu;
class Boot;

class CpuBus final : public Bus {

public:
#ifdef ENABLE_BOOTROM
    CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
           Sound& sound, Ppu& ppu, Boot& boot);
#else
    CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Sound& sound, Ppu& ppu,
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
    Sound& sound;
    Boot& boot;

    Ppu& ppu;
};
#endif // CPUBUS_H