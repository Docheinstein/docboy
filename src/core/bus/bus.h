#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include "core/definitions.h"
#include "core/boot/bootrom.h"

class IMemory;
class IJoypadIO;
class ISerialIO;
class ITimersIO;
class IInterruptsIO;
class ISoundIO;
class ILCDIO;
class IBootIO;
class ICartridge;

class IBus {
public:
    virtual ~IBus() = default;
    [[nodiscard]] virtual uint8_t read(uint16_t addr) const = 0;
    virtual void write(uint16_t addr, uint8_t value) = 0;
};

class Bus : public IBus {
public:
    Bus(IMemory &vram,
        IMemory &wram1,
        IMemory &wram2,
        IMemory &oam,
        IMemory &hram,
        ICartridge &cartridge,
        IBootROM &bootrom,
        IJoypadIO &joypad,
        ISerialIO &serial,
        ITimersIO &timers,
        IInterruptsIO &interrupts,
        ISoundIO &sound,
        ILCDIO &lcd,
        IBootIO &boot);

    ~Bus() override = default;

    [[nodiscard]] uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;

private:
    IMemory &vram;
    IMemory &wram1;
    IMemory &wram2;
    IMemory &oam;
    IMemory &hram;

    ICartridge &cartridge;
    IBootROM &bootRom;

    IJoypadIO &joypad;
    ISerialIO &serial;
    ITimersIO &timers;
    IInterruptsIO &interrupts;
    ISoundIO &sound;
    ILCDIO &lcd;
    IBootIO &boot;

    typedef uint8_t (Bus::*Reader)() const;
    typedef void (Bus::*Writer)(uint8_t);
    struct IOHandler {
        Reader read;
        Writer write;
    };

    IOHandler io[MemoryMap::IO::SIZE];

    // Joypad
    [[nodiscard]] uint8_t readP1() const;
    void writeP1(uint8_t value);

    // Serial
    [[nodiscard]] uint8_t readSB() const;
    void writeSB(uint8_t value);

    [[nodiscard]] uint8_t readSC() const;
    void writeSC(uint8_t value);

    // Timers
    [[nodiscard]] uint8_t readDIV() const;
    void writeDIV(uint8_t value);

    [[nodiscard]] uint8_t readTIMA() const;
    void writeTIMA(uint8_t value);

    [[nodiscard]] uint8_t readTMA() const;
    void writeTMA(uint8_t value);

    [[nodiscard]] uint8_t readTAC() const;
    void writeTAC(uint8_t value);

    // Interrupts
    [[nodiscard]] uint8_t readIF() const;
    void writeIF(uint8_t value);

    [[nodiscard]] virtual uint8_t readIE() const;
    virtual void writeIE(uint8_t value);

    // Sound
    [[nodiscard]] uint8_t readNR10() const;
    void writeNR10(uint8_t value);

    [[nodiscard]] uint8_t readNR11() const;
    void writeNR11(uint8_t value);

    [[nodiscard]] uint8_t readNR12() const;
    void writeNR12(uint8_t value);

    [[nodiscard]] uint8_t readNR13() const;
    void writeNR13(uint8_t value);

    [[nodiscard]] uint8_t readNR14() const;
    void writeNR14(uint8_t value);

    [[nodiscard]] uint8_t readNR21() const;
    void writeNR21(uint8_t value);

    [[nodiscard]] uint8_t readNR22() const;
    void writeNR22(uint8_t value);

    [[nodiscard]] uint8_t readNR23() const;
    void writeNR23(uint8_t value);

    [[nodiscard]] uint8_t readNR24() const;
    void writeNR24(uint8_t value);

    [[nodiscard]] uint8_t readNR30() const;
    void writeNR30(uint8_t value);

    [[nodiscard]] uint8_t readNR31() const;
    void writeNR31(uint8_t value);

    [[nodiscard]] uint8_t readNR32() const;
    void writeNR32(uint8_t value);

    [[nodiscard]] uint8_t readNR33() const;
    void writeNR33(uint8_t value);

    [[nodiscard]] uint8_t readNR34() const;
    void writeNR34(uint8_t value);

    [[nodiscard]] uint8_t readNR41() const;
    void writeNR41(uint8_t value);

    [[nodiscard]] uint8_t readNR42() const;
    void writeNR42(uint8_t value);

    [[nodiscard]] uint8_t readNR43() const;
    void writeNR43(uint8_t value);

    [[nodiscard]] uint8_t readNR44() const;
    void writeNR44(uint8_t value);

    [[nodiscard]] uint8_t readNR50() const;
    void writeNR50(uint8_t value);

    [[nodiscard]] uint8_t readNR51() const;
    void writeNR51(uint8_t value);

    [[nodiscard]] uint8_t readNR52() const;
    void writeNR52(uint8_t value);

    [[nodiscard]] uint8_t readWAVE0() const;
    void writeWAVE0(uint8_t value);

    [[nodiscard]] uint8_t readWAVE1() const;
    void writeWAVE1(uint8_t value);

    [[nodiscard]] uint8_t readWAVE2() const;
    void writeWAVE2(uint8_t value);

    [[nodiscard]] uint8_t readWAVE3() const;
    void writeWAVE3(uint8_t value);

    [[nodiscard]] uint8_t readWAVE4() const;
    void writeWAVE4(uint8_t value);

    [[nodiscard]] uint8_t readWAVE5() const;
    void writeWAVE5(uint8_t value);

    [[nodiscard]] uint8_t readWAVE6() const;
    void writeWAVE6(uint8_t value);

    [[nodiscard]] uint8_t readWAVE7() const;
    void writeWAVE7(uint8_t value);

    [[nodiscard]] uint8_t readWAVE8() const;
    void writeWAVE8(uint8_t value);

    [[nodiscard]] uint8_t readWAVE9() const;
    void writeWAVE9(uint8_t value);

    [[nodiscard]] uint8_t readWAVEA() const;
    void writeWAVEA(uint8_t value);

    [[nodiscard]] uint8_t readWAVEB() const;
    void writeWAVEB(uint8_t value);

    [[nodiscard]] uint8_t readWAVEC() const;
    void writeWAVEC(uint8_t value);

    [[nodiscard]] uint8_t readWAVED() const;
    void writeWAVED(uint8_t value);

    [[nodiscard]] uint8_t readWAVEE() const;
    void writeWAVEE(uint8_t value);

    [[nodiscard]] uint8_t readWAVEF() const;
    void writeWAVEF(uint8_t value);

    // LCD
    [[nodiscard]] uint8_t readLCDC() const;
    void writeLCDC(uint8_t value);

    [[nodiscard]] uint8_t readSTAT() const;
    void writeSTAT(uint8_t value);

    [[nodiscard]] uint8_t readSCY() const;
    void writeSCY(uint8_t value);

    [[nodiscard]] uint8_t readSCX() const;
    void writeSCX(uint8_t value);

    [[nodiscard]] uint8_t readLY() const;
    void writeLY(uint8_t value);

    [[nodiscard]] uint8_t readLYC() const;
    void writeLYC(uint8_t value);

    [[nodiscard]] uint8_t readDMA() const;
    void writeDMA(uint8_t value);

    [[nodiscard]] uint8_t readBGP() const;
    void writeBGP(uint8_t value);

    [[nodiscard]] uint8_t readOBP0() const;
    void writeOBP0(uint8_t value);

    [[nodiscard]] uint8_t readOBP1() const;
    void writeOBP1(uint8_t value);

    [[nodiscard]] uint8_t readWY() const;
    void writeWY(uint8_t value);

    [[nodiscard]] uint8_t readWX() const;
    void writeWX(uint8_t value);

    // Boot
    [[nodiscard]] uint8_t readBOOT() const;
    void writeBOOT(uint8_t value);

    [[nodiscard]] uint8_t readNull() const;
    void writeNull(uint8_t value);
};

#endif // BUS_H