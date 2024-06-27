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
class SoundIO;
class Ppu;
class BootIO;

class CpuBus final : public Bus {

public:
#ifdef ENABLE_BOOTROM
    CpuBus(BootRom& boot_rom, Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers,
           InterruptsIO& interrupts, SoundIO& sound, Ppu& ppu, BootIO& boot);
#else
    CpuBus(Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers, InterruptsIO& interrupts, SoundIO& sound,
           Ppu& ppu, BootIO& boot);
#endif

#ifdef ENABLE_BOOTROM
    BootRom& boot_rom;
#endif

    Hram& hram;

    JoypadIO& joypad;
    SerialIO& serial;
    TimersIO& timers;
    InterruptsIO& interrupts;
    SoundIO& sound;
    BootIO& boot;

    Ppu& ppu;
};
#endif // CPUBUS_H