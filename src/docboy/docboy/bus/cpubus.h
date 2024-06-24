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

class CpuBus final : public Bus<CpuBus> {

public:
#ifdef ENABLE_BOOTROM
    CpuBus(BootRom& boot_rom, Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers,
           InterruptsIO& interrupts, Apu& apu, Ppu& ppu, BootIO& boot);
#else
    CpuBus(Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers, InterruptsIO& interrupts, Apu& apu,
           Ppu& Ppu, BootIO& boot);
#endif

private:
    uint8_t read_p1(uint16_t address) const;
    void write_p1(uint16_t address, uint8_t value);

    void write_sc(uint16_t address, uint8_t value);

    uint8_t read_div(uint16_t address) const;
    void write_div(uint16_t address, uint8_t value);
    void write_tima(uint16_t address, uint8_t value);
    void write_tma(uint16_t address, uint8_t value);
    void write_tac(uint16_t address, uint8_t value);

    void write_if(uint16_t address, uint8_t value);

    void write_nr10(uint16_t address, uint8_t value);
    uint8_t read_nr21(uint16_t address) const;
    void write_nr21(uint16_t address, uint8_t value);
    uint8_t read_nr22(uint16_t address) const;
    void write_nr22(uint16_t address, uint8_t value);
    uint8_t read_nr24(uint16_t address) const;
    void write_nr24(uint16_t address, uint8_t value);
    void write_nr30(uint16_t address, uint8_t value);
    void write_nr32(uint16_t address, uint8_t value);
    void write_nr41(uint16_t address, uint8_t value);
    void write_nr44(uint16_t address, uint8_t value);
    uint8_t read_nr52(uint16_t address) const;
    void write_nr52(uint16_t address, uint8_t value);

    uint8_t read_lcdc(uint16_t address) const;
    void write_lcdc(uint16_t address, uint8_t value);
    uint8_t read_stat(uint16_t address) const;
    void write_stat(uint16_t address, uint8_t value);
    void write_dma(uint16_t address, uint8_t value);

    void write_boot(uint16_t address, uint8_t value);

    uint8_t read_ff(uint16_t address) const;
    void write_nop(uint16_t address, uint8_t value);

#ifdef ENABLE_BOOTROM
    BootRom& boot_rom;
#endif

    Hram& hram;

    struct {
        JoypadIO& joypad;
        SerialIO& serial;
        TimersIO& timers;
        InterruptsIO& interrupts;
        BootIO& boot;
    } io;

    Apu& apu;
    Ppu& ppu;
};
#endif // CPUBUS_H