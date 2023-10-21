#ifndef BUS_H
#define BUS_H

#include <cstdint>

#include "docboy/bootrom/macros.h"
#include "docboy/memory/fwd/bytefwd.h"
#include "docboy/memory/fwd/hramfwd.h"
#include "docboy/memory/fwd/memoryfwd.h"
#include "docboy/memory/fwd/oamfwd.h"
#include "docboy/memory/fwd/vramfwd.h"
#include "docboy/memory/fwd/wram1fwd.h"
#include "docboy/memory/fwd/wram2fwd.h"
#include "utils/macros.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/fwd/bootromfwd.h"
#endif

class CartridgeSlot;
class JoypadIO;
class SerialIO;
class TimersIO;
class InterruptsIO;
class SoundIO;
class VideoIO;
class BootIO;

class Bus {
public:
    Bus(IF_BOOTROM(BootRom& bootRom COMMA) CartridgeSlot& cartridgeSlot, Vram& vram, Wram1& wram1, Wram2& wram2,
        Oam& oam, Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers, InterruptsIO& interrupts,
        SoundIO& sound, VideoIO& video, BootIO& boot);

    [[nodiscard]] uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

private:
    using NonTrivialMemoryRead = uint8_t (Bus::*)(uint16_t) const;
    using NonTrivialMemoryWrite = void (Bus::*)(uint16_t, uint8_t);

    struct MemoryAccess {
        MemoryAccess() = default;
        MemoryAccess(byte* rw);
        MemoryAccess(byte* r, byte* w);
        MemoryAccess(NonTrivialMemoryRead r, byte* w);
        MemoryAccess(byte* r, NonTrivialMemoryWrite w);
        MemoryAccess(NonTrivialMemoryRead r, NonTrivialMemoryWrite w);

        struct Read {
            NonTrivialMemoryRead nonTrivial {};
            byte* trivial {};
        } read {};
        struct Write {
            NonTrivialMemoryWrite nonTrivial {};
            byte* trivial {};
        } write {};
    };

    [[nodiscard]] uint8_t readCartridgeRom(uint16_t address) const;
    void writeCartridgeRom(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readCartridgeRam(uint16_t address) const;
    void writeCartridgeRam(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readVram(uint16_t address) const;
    void writeVram(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readOam(uint16_t address) const;
    void writeOam(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readP1(uint16_t address) const;
    void writeP1(uint16_t address, uint8_t value);

    void writeSC(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readDIV(uint16_t address) const;
    void writeDIV(uint16_t address, uint8_t value);
    void writeTIMA(uint16_t address, uint8_t value);
    void writeTMA(uint16_t address, uint8_t value);
    void writeTAC(uint16_t address, uint8_t value);

    void writeIF(uint16_t address, uint8_t value);

    void writeNR10(uint16_t address, uint8_t value);
    void writeNR30(uint16_t address, uint8_t value);
    void writeNR32(uint16_t address, uint8_t value);
    void writeNR41(uint16_t address, uint8_t value);
    void writeNR44(uint16_t address, uint8_t value);
    void writeNR52(uint16_t address, uint8_t value);

    void writeSTAT(uint16_t address, uint8_t value);

    void writeDMA(uint16_t address, uint8_t value);

    void writeBOOT(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readFF(uint16_t address) const;
    void writeNop(uint16_t address, uint8_t value);

    IF_BOOTROM(BootRom& bootRom);
    CartridgeSlot& cartridgeSlot;
    Vram& vram;
    Wram1& wram1;
    Wram2& wram2;
    Oam& oam;
    Hram& hram;
    JoypadIO& joypad;
    SerialIO& serial;
    TimersIO& timers;
    InterruptsIO& interrupts;
    SoundIO& sound;
    VideoIO& video;
    BootIO& boot;

    MemoryAccess memoryAccessors[UINT16_MAX + 1] {};
};
#endif // BUS_H