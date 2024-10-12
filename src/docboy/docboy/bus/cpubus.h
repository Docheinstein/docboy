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
class VramBankController;
class WramBankController;
class Infrared;
class Boot;

class CpuBus final : public Bus {

public:
#ifdef ENABLE_BOOTROM
#ifdef ENABLE_CGB
    CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
           Apu& apu, Ppu& ppu, VramBankController& vram_bank_controller, WramBankController& wram_bank_controller,
           Infrared& infrared, Boot& boot);
#else
    CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
           Apu& apu, Ppu& ppu, Boot& boot);
#endif
#else
#ifdef ENABLE_CGB
    CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Apu& apu, Ppu& ppu,
           VramBankController& vram_bank_controller, WramBankController& wram_bank_controller, Infrared& infrared,
           Boot& boot);
#else
    CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Apu& apu, Ppu& ppu,
           Boot& boot);
#endif
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

#ifdef ENABLE_CGB
    VramBankController& vram_bank_controller;
    WramBankController& wram_bank_controller;

    Infrared& infrared;
#endif

    Apu& apu;
    Ppu& ppu;
};
#endif // CPUBUS_H