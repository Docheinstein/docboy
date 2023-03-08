#ifndef IO_H
#define IO_H

#include "core/impl/memory.h"

class IIO {
public:
    virtual ~IIO() = default;

    // Joypad
    [[nodiscard]] virtual uint8_t readP1() const = 0;
    virtual void writeP1(uint8_t value) = 0;

    // Serial
    [[nodiscard]] virtual uint8_t readSB() const = 0;
    virtual void writeSB(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readSC() const = 0;
    virtual void writeSC(uint8_t value) = 0;

    // Timers
    [[nodiscard]] virtual uint8_t readDIV() const = 0;
    virtual void writeDIV(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readTIMA() const = 0;
    virtual void writeTIMA(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readTMA() const = 0;
    virtual void writeTMA(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readTAC() const = 0;
    virtual void writeTAC(uint8_t value) = 0;

    // Interrupts
    [[nodiscard]] virtual uint8_t readIF() const = 0;
    virtual void writeIF(uint8_t value) = 0;

    // Sound
    [[nodiscard]] virtual uint8_t readNR10() const = 0;
    virtual void writeNR10(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR11() const = 0;
    virtual void writeNR11(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR12() const = 0;
    virtual void writeNR12(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR13() const = 0;
    virtual void writeNR13(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR14() const = 0;
    virtual void writeNR14(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR21() const = 0;
    virtual void writeNR21(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR22() const = 0;
    virtual void writeNR22(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR23() const = 0;
    virtual void writeNR23(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR24() const = 0;
    virtual void writeNR24(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR30() const = 0;
    virtual void writeNR30(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR31() const = 0;
    virtual void writeNR31(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR32() const = 0;
    virtual void writeNR32(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR33() const = 0;
    virtual void writeNR33(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR34() const = 0;
    virtual void writeNR34(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR41() const = 0;
    virtual void writeNR41(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR42() const = 0;
    virtual void writeNR42(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR43() const = 0;
    virtual void writeNR43(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR44() const = 0;
    virtual void writeNR44(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR50() const = 0;
    virtual void writeNR50(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR51() const = 0;
    virtual void writeNR51(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readNR52() const = 0;
    virtual void writeNR52(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE0() const = 0;
    virtual void writeWAVE0(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE1() const = 0;
    virtual void writeWAVE1(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE2() const = 0;
    virtual void writeWAVE2(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE3() const = 0;
    virtual void writeWAVE3(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE4() const = 0;
    virtual void writeWAVE4(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE5() const = 0;
    virtual void writeWAVE5(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE6() const = 0;
    virtual void writeWAVE6(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE7() const = 0;
    virtual void writeWAVE7(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE8() const = 0;
    virtual void writeWAVE8(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVE9() const = 0;
    virtual void writeWAVE9(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVEA() const = 0;
    virtual void writeWAVEA(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVEB() const = 0;
    virtual void writeWAVEB(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVEC() const = 0;
    virtual void writeWAVEC(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVED() const = 0;
    virtual void writeWAVED(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVEE() const = 0;
    virtual void writeWAVEE(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWAVEF() const = 0;
    virtual void writeWAVEF(uint8_t value) = 0;

    // LCD
    [[nodiscard]] virtual uint8_t readLCDC() const = 0;
    virtual void writeLCDC(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readSTAT() const = 0;
    virtual void writeSTAT(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readSCY() const = 0;
    virtual void writeSCY(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readSCX() const = 0;
    virtual void writeSCX(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readLY() const = 0;
    virtual void writeLY(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readLYC() const = 0;
    virtual void writeLYC(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readDMA() const = 0;
    virtual void writeDMA(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readBGP() const = 0;
    virtual void writeBGP(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readOBP0() const = 0;
    virtual void writeOBP0(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readOBP1() const = 0;
    virtual void writeOBP1(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWY() const = 0;
    virtual void writeWY(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWX() const = 0;
    virtual void writeWX(uint8_t value) = 0;

    // Boot
    [[nodiscard]] virtual uint8_t readBOOTROM() const = 0;
    virtual void writeBOOTROM(uint8_t value) = 0;
};

class IO : public Impl::Memory, public IIO {
public:
    IO();

    [[nodiscard]] [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

    // Joypad
    [[nodiscard]] uint8_t readP1() const override;
    void writeP1(uint8_t value) override;

    // Serial
    [[nodiscard]] uint8_t readSB() const override;
    void writeSB(uint8_t value) override;

    [[nodiscard]] uint8_t readSC() const override;
    void writeSC(uint8_t value) override;

    // Timers
    [[nodiscard]] uint8_t readDIV() const override;
    void writeDIV(uint8_t value) override;

    [[nodiscard]] uint8_t readTIMA() const override;
    void writeTIMA(uint8_t value) override;

    [[nodiscard]] uint8_t readTMA() const override;
    void writeTMA(uint8_t value) override;

    [[nodiscard]] uint8_t readTAC() const override;
    void writeTAC(uint8_t value) override;

    // Interrupts
    [[nodiscard]] uint8_t readIF() const override;
    void writeIF(uint8_t value) override;

    // Sound
    [[nodiscard]] uint8_t readNR10() const override;
    void writeNR10(uint8_t value) override;

    [[nodiscard]] uint8_t readNR11() const override;
    void writeNR11(uint8_t value) override;

    [[nodiscard]] uint8_t readNR12() const override;
    void writeNR12(uint8_t value) override;

    [[nodiscard]] uint8_t readNR13() const override;
    void writeNR13(uint8_t value) override;

    [[nodiscard]] uint8_t readNR14() const override;
    void writeNR14(uint8_t value) override;

    [[nodiscard]] uint8_t readNR21() const override;
    void writeNR21(uint8_t value) override;

    [[nodiscard]] uint8_t readNR22() const override;
    void writeNR22(uint8_t value) override;

    [[nodiscard]] uint8_t readNR23() const override;
    void writeNR23(uint8_t value) override;

    [[nodiscard]] uint8_t readNR24() const override;
    void writeNR24(uint8_t value) override;

    [[nodiscard]] uint8_t readNR30() const override;
    void writeNR30(uint8_t value) override;

    [[nodiscard]] uint8_t readNR31() const override;
    void writeNR31(uint8_t value) override;

    [[nodiscard]] uint8_t readNR32() const override;
    void writeNR32(uint8_t value) override;

    [[nodiscard]] uint8_t readNR33() const override;
    void writeNR33(uint8_t value) override;

    [[nodiscard]] uint8_t readNR34() const override;
    void writeNR34(uint8_t value) override;

    [[nodiscard]] uint8_t readNR41() const override;
    void writeNR41(uint8_t value) override;

    [[nodiscard]] uint8_t readNR42() const override;
    void writeNR42(uint8_t value) override;

    [[nodiscard]] uint8_t readNR43() const override;
    void writeNR43(uint8_t value) override;

    [[nodiscard]] uint8_t readNR44() const override;
    void writeNR44(uint8_t value) override;

    [[nodiscard]] uint8_t readNR50() const override;
    void writeNR50(uint8_t value) override;

    [[nodiscard]] uint8_t readNR51() const override;
    void writeNR51(uint8_t value) override;

    [[nodiscard]] uint8_t readNR52() const override;
    void writeNR52(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE0() const override;
    void writeWAVE0(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE1() const override;
    void writeWAVE1(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE2() const override;
    void writeWAVE2(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE3() const override;
    void writeWAVE3(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE4() const override;
    void writeWAVE4(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE5() const override;
    void writeWAVE5(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE6() const override;
    void writeWAVE6(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE7() const override;
    void writeWAVE7(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE8() const override;
    void writeWAVE8(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVE9() const override;
    void writeWAVE9(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVEA() const override;
    void writeWAVEA(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVEB() const override;
    void writeWAVEB(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVEC() const override;
    void writeWAVEC(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVED() const override;
    void writeWAVED(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVEE() const override;
    void writeWAVEE(uint8_t value) override;

    [[nodiscard]] uint8_t readWAVEF() const override;
    void writeWAVEF(uint8_t value) override;

    // LCD
        [[nodiscard]] uint8_t readLCDC() const override;
    void writeLCDC(uint8_t value) override;

    [[nodiscard]] uint8_t readSTAT() const override;
    void writeSTAT(uint8_t value) override;

    [[nodiscard]] uint8_t readSCY() const override;
    void writeSCY(uint8_t value) override;

    [[nodiscard]] uint8_t readSCX() const override;
    void writeSCX(uint8_t value) override;

    [[nodiscard]] uint8_t readLY() const override;
    void writeLY(uint8_t value) override;

    [[nodiscard]] uint8_t readLYC() const override;
    void writeLYC(uint8_t value) override;

    [[nodiscard]] uint8_t readDMA() const override;
    void writeDMA(uint8_t value) override;

    [[nodiscard]] uint8_t readBGP() const override;
    void writeBGP(uint8_t value) override;

    [[nodiscard]] uint8_t readOBP0() const override;
    void writeOBP0(uint8_t value) override;

    [[nodiscard]] uint8_t readOBP1() const override;
    void writeOBP1(uint8_t value) override;

    [[nodiscard]] uint8_t readWY() const override;
    void writeWY(uint8_t value) override;

    [[nodiscard]] uint8_t readWX() const override;
    void writeWX(uint8_t value) override;

    // Boot
    [[nodiscard]] uint8_t readBOOTROM() const override;
    void writeBOOTROM(uint8_t value) override;
};



#endif // IO_H