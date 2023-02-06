#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <string>
#include <memory>
#include <iostream>
#include "bus.h"

class CPU {
public:
    explicit CPU(Bus &bus);

    void tick();

private:
    enum Register8 {
        REG_A,
        REG_B,
        REG_C,
        REG_D,
        REG_E,
        REG_F,
        REG_H,
        REG_L,
    };

    enum Register16 {
        REG_AF,
        REG_BC,
        REG_DE,
        REG_HL,
    };

    enum Flag {
        FLAG_Z = 7,
        FLAG_N = 6,
        FLAG_H = 5,
        FLAG_C = 4,
    };

    Bus &bus;

    uint16_t AF;
    uint16_t BC;
    uint16_t DE;
    uint16_t HL;
    uint16_t SP;
    uint16_t PC;

    uint64_t mCycles;

    struct {
        uint8_t instruction;
        uint8_t m;
    } currentInstruction;

    uint8_t tmp1;
    uint8_t tmp2;

    typedef void (CPU::*InstructionMicroOperation)();
    InstructionMicroOperation instructions[256][6];

    [[nodiscard]] std::string status() const;

    template<Register8 reg>
    [[nodiscard]] uint8_t readRegister8() const;

    template<Register8 reg>
    void writeRegister8(uint8_t value);

    template<Register16 reg>
    [[nodiscard]] uint16_t readRegister16() const;

    template<Register16 reg>
    void writeRegister16(uint16_t value);

    template<Flag flag>
    [[nodiscard]] bool readFlag() const;

    template<Flag flag>
    void writeFlag(bool value);

    void instructionNotImplemented();
    void fetch();

    void NOP_m1();

    void JP_a16_m1();
    void JP_a16_m2();
    void JP_a16_m3();
    void JP_a16_m4();

    template<Register8 reg>
    void XORr_m1();

    template<Register8 reg>
    void XORr_m2();

    template<Register16 reg>
    void XORa_m1();

    template<Register16 reg>
    void XORa_m2();

    template<Register16 reg>
    void XORa_m3();

};

#endif // CPU_H