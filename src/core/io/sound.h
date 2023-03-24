#ifndef SOUNDIO_H
#define SOUNDIO_H

#include <cstdint>

class ISoundIO {
public:
    virtual ~ISoundIO() = default;

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
};

#endif // SOUNDIO_H