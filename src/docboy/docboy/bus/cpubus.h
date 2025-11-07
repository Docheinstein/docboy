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
class OperatingMode;
class SpeedSwitch;
class Hdma;
class Infrared;
class UndocumentedRegisters;
class Boot;

class CpuBus final : public Bus {

public:
    template <Device::Type Dev>
    using View = BusView<CpuBus, Dev>;

#ifdef ENABLE_BOOTROM
#ifdef ENABLE_CGB
    CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
           Boot& boot, Apu& apu, Ppu& ppu, Dma& dma, VramBankController& vram_bank_controller,
           WramBankController& wram_bank_controller, Hdma& hdma, OperatingMode& operating_mode,
           SpeedSwitch& speed_switch, Infrared& infrared, UndocumentedRegisters& undocumented_registers);
#else
    CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
           Boot& boot, Apu& apu, Ppu& ppu, Dma& dma);
#endif
#else
#ifdef ENABLE_CGB
    CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Boot& boot, Apu& apu,
           Ppu& ppu, Dma& dma, VramBankController& vram_bank_controller, WramBankController& wram_bank_controller,
           Hdma& hdma, OperatingMode& operating_mode, SpeedSwitch& speed_switch, Infrared& infrared,
           UndocumentedRegisters& undocumented_registers);
#else
    CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Boot& boot, Apu& apu,
           Ppu& ppu, Dma& dma);
#endif
#endif

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

#if defined(ENABLE_CGB) && defined(ENABLE_BOOTROM)
    void lock_boot_rom();
#endif

private:
#ifdef ENABLE_CGB
    void init_accessors_for_operating_mode();
#endif

    static constexpr NonTrivialReadFunctor READ_FF {[](void*, uint16_t) -> uint8_t {
                                                        return 0xFF;
                                                    },
                                                    nullptr};
    static constexpr NonTrivialWriteFunctor WRITE_NOP {[](void*, uint16_t, uint8_t) {
                                                       },
                                                       nullptr};
    static constexpr MemoryAccess OPEN_BUS {READ_FF, WRITE_NOP};

#if defined(ENABLE_CGB) && defined(ENABLE_BOOTROM)
    bool boot_rom_locked {};
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
    VramBankController& vram_bank_controller;
    WramBankController& wram_bank_controller;

    Hdma& hdma;

    OperatingMode& operating_mode;
    SpeedSwitch& speed_switch;

    Infrared& infrared;
    UndocumentedRegisters& undocumented_registers;
#endif
};
#endif // CPUBUS_H
