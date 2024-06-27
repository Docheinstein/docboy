#ifndef CPUBUS_H
#define CPUBUS_H

#include "docboy/bus/bus.h"
#include "docboy/memory/fwd/hramfwd.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/fwd/bootromfwd.h"
#endif

class JoypadIO;
class SerialIO;
class TimersIO;
class InterruptsIO;
class BootIO;
class Apu;
class Ppu;

class CpuBus final : public Bus {

public:
#ifdef ENABLE_BOOTROM
    CpuBus(BootRom& boot_rom, Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers,
           InterruptsIO& interrupts, Apu& apu, Ppu& ppu, BootIO& boot);
#else
    CpuBus(Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers, InterruptsIO& interrupts, Apu& apu,
           Ppu& Ppu, BootIO& boot);
#endif

#ifdef ENABLE_BOOTROM
    BootRom& boot_rom;
#endif

    Hram& hram;

    JoypadIO& joypad;
    SerialIO& serial;
    TimersIO& timers;
    InterruptsIO& interrupts;
    BootIO& boot;

    Apu& apu;
    Ppu& ppu;
};
#endif // CPUBUS_H