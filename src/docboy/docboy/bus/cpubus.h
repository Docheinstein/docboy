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
class Dma;
class Apu;
class Ppu;
class VramBankController;
class WramBankController;
class Hdma;
class Infrared;
class UndocumentedRegisters;
class Boot;

class CpuBus final : public Bus {

public:
#ifdef ENABLE_BOOTROM
#ifdef ENABLE_CGB
    CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
           Boot& boot, Apu& apu, Ppu& ppu, Dma& dma, Hdma& hdma, VramBankController& vram_bank_controller,
           WramBankController& wram_bank_controller, Infrared& infrared, UndocumentedRegisters& undocumented_registers);
#else
    CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
           Boot& boot, Apu& apu, Ppu& ppu, Dma& dma);
#endif
#else
#ifdef ENABLE_CGB
    CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Boot& boot, Apu& apu,
           Ppu& ppu, Dma& dma, Hdma& hdma, VramBankController& vram_bank_controller,
           WramBankController& wram_bank_controller, Infrared& infrared, UndocumentedRegisters& undocumented_registers);
#else
    CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Boot& boot, Apu& apu,
           Ppu& ppu, Dma& dma);
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

    Apu& apu;
    Ppu& ppu;

    Dma& dma;

#ifdef ENABLE_CGB
    Hdma& hdma;

    VramBankController& vram_bank_controller;
    WramBankController& wram_bank_controller;

    Infrared& infrared;
    UndocumentedRegisters& undocumented_registers;
#endif
};
#endif // CPUBUS_H