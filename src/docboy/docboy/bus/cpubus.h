#ifndef CPUBUS_H
#define CPUBUS_H

#include "bus.h"
#include "docboy/bootrom/fwd/bootromfwd.h"
#include "docboy/bootrom/macros.h"
#include "docboy/memory/fwd/hramfwd.h"
#include "utils/macros.h"

class JoypadIO;
class SerialIO;
class TimersIO;
class InterruptsIO;
class SoundIO;
class VideoIO;
class BootIO;

class CpuBus : public Bus<CpuBus> {

public:
    CpuBus(IF_BOOTROM(BootRom& bootRom COMMA) Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers,
           InterruptsIO& interrupts, SoundIO& sound, VideoIO& video, BootIO& boot);

private:
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
    Hram& hram;
    struct {
        JoypadIO& joypad;
        SerialIO& serial;
        TimersIO& timers;
        InterruptsIO& interrupts;
        SoundIO& sound;
        VideoIO& video;
        BootIO& boot;
    } io;
};
#endif // CPUBUS_H