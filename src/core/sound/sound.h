#ifndef SOUND_H
#define SOUND_H

#include "core/io/sound.h"

class Sound : public ISoundIO {
public:
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

private:
    uint8_t NR10;
    uint8_t NR11;
    uint8_t NR12;
    uint8_t NR13;
    uint8_t NR14;
    uint8_t NR21;
    uint8_t NR22;
    uint8_t NR23;
    uint8_t NR24;
    uint8_t NR30;
    uint8_t NR31;
    uint8_t NR32;
    uint8_t NR33;
    uint8_t NR34;
    uint8_t NR41;
    uint8_t NR42;
    uint8_t NR43;
    uint8_t NR44;
    uint8_t NR50;
    uint8_t NR51;
    uint8_t NR52;
    uint8_t WAVE0;
    uint8_t WAVE1;
    uint8_t WAVE2;
    uint8_t WAVE3;
    uint8_t WAVE4;
    uint8_t WAVE5;
    uint8_t WAVE6;
    uint8_t WAVE7;
    uint8_t WAVE8;
    uint8_t WAVE9;
    uint8_t WAVEA;
    uint8_t WAVEB;
    uint8_t WAVEC;
    uint8_t WAVED;
    uint8_t WAVEE;
    uint8_t WAVEF;
};

#endif // SOUND_H