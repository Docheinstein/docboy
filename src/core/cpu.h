#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <string>
#include <memory>
#include <iostream>
#include "bus.h"

class CPU {
public:
    class InstructionNotImplementedException : public std::logic_error {
    public:
        explicit InstructionNotImplementedException(const std::string &what);
    };
    class IllegalInstructionException : public std::logic_error {
    public:
        explicit IllegalInstructionException(const std::string &what);
    };

    explicit CPU(IBus &bus);

    void reset();
    void tick();

    [[nodiscard]] uint16_t getAF() const;
    [[nodiscard]] uint16_t getBC() const;
    [[nodiscard]] uint16_t getDE() const;
    [[nodiscard]] uint16_t getHL() const;
    [[nodiscard]] uint16_t getPC() const;
    [[nodiscard]] uint16_t getSP() const;
    [[nodiscard]] uint8_t getCurrentInstructionOpcode() const;
    [[nodiscard]] uint8_t getCurrentInstructionMicroOperation() const;

private:
    enum class Register8 {
        A,
        B,
        C,
        D,
        E,
        F,
        H,
        L,
        S,
        P,
    };

    enum class Register16 {
        AF,
        BC,
        DE,
        HL,
        PC,
        SP,
    };

    enum class Flag {
        Z,
        N,
        H,
        C,
    };

    IBus &bus;

    uint16_t AF;
    uint16_t BC;
    uint16_t DE;
    uint16_t HL;
    uint16_t PC;
    uint16_t SP;

    bool IME;

    uint64_t mCycles;

    struct {
        uint8_t opcode;
        uint8_t microop;
    } currentInstruction;

    uint8_t u1;
    uint8_t u2;
    uint16_t uu1;

    typedef void (CPU::*InstructionMicroOperation)();
    InstructionMicroOperation instructions[256][6];

    [[nodiscard]] std::string status() const;

    template<Register8 r>
    [[nodiscard]] uint8_t readRegister8() const;

    template<Register8 r>
    void writeRegister8(uint8_t value);

    template<Register16 rr>
    [[nodiscard]] uint16_t readRegister16() const;

    template<Register16 rr>
    void writeRegister16(uint16_t value);

    template<Flag f>
    [[nodiscard]] bool readFlag() const;

    template<Flag f>
    void writeFlag(bool value);

    void instructionNotImplemented();
    void fetch();

    /*
     * r: 8 bit register
     * rr: 16 bit register
     * n: 8 bit immediate number
     * nn: 16 bit immediate number
     * ar: address specified by 8 bit register
     * arr: address specified by 16 bit register
     * an: address specified by 8 bit immediate number
     * ann: address specified by 16 bit immediate number
     */

    void NOP_m1();
    void STOP_m1();
    void HALT_m1();
    void DI_m1();
    void EI_m1();

    template<Register16 rr>
    void LD_rr_nn_m1();
    template<Register16 rr>
    void LD_rr_nn_m2();
    template<Register16 rr>
    void LD_rr_nn_m3();

    template<Register16 rr>
    void LD_arr_n_m1();
    template<Register16 rr>
    void LD_arr_n_m2();
    template<Register16 rr>
    void LD_arr_n_m3();

    template<Register16 rr, Register8 r>
    void LD_arr_r_m1();
    template<Register16 rr, Register8 r>
    void LD_arr_r_m2();

    template<Register16 rr, Register8 r, int8_t inc>
    void LD_arr_r_m1();
    template<Register16 rr, Register8 r, int8_t inc>
    void LD_arr_r_m2();

    template<Register8 r>
    void LD_r_n_m1();
    template<Register8 r>
    void LD_r_n_m2();

    template<Register16 rr>
    void LD_ann_rr_m1();
    template<Register16 rr>
    void LD_ann_rr_m2();
    template<Register16 rr>
    void LD_ann_rr_m3();
    template<Register16 rr>
    void LD_ann_rr_m4();
    template<Register16 rr>
    void LD_ann_rr_m5();

    template<Register8 r, Register16 rr>
    void LD_r_arr_m1();
    template<Register8 r, Register16 rr>
    void LD_r_arr_m2();

    template<Register8 r, Register16 rr, int8_t inc>
    void LD_r_arr_m1();
    template<Register8 r, Register16 rr, int8_t inc>
    void LD_r_arr_m2();

    template<Register8 r1, Register8 r2>
    void LD_r_r_m1();

    template<Register8 r>
    void LDH_an_r_m1();
    template<Register8 r>
    void LDH_an_r_m2();
    template<Register8 r>
    void LDH_an_r_m3();

    template<Register8 r>
    void LDH_r_an_m1();
    template<Register8 r>
    void LDH_r_an_m2();
    template<Register8 r>
    void LDH_r_an_m3();

    template<Register8 r1, Register8 r2>
    void LDH_ar_r_m1();
    template<Register8 r1, Register8 r2>
    void LDH_ar_r_m2();

    template<Register8 r1, Register8 r2>
    void LDH_r_ar_m1();
    template<Register8 r1, Register8 r2>
    void LDH_r_ar_m2();

    template<Register8 r>
    void LD_ann_r_m1();
    template<Register8 r>
    void LD_ann_r_m2();
    template<Register8 r>
    void LD_ann_r_m3();
    template<Register8 r>
    void LD_ann_r_m4();

    template<Register8 r>
    void LD_r_ann_m1();
    template<Register8 r>
    void LD_r_ann_m2();
    template<Register8 r>
    void LD_r_ann_m3();
    template<Register8 r>
    void LD_r_ann_m4();

    template<Register16 rr1, Register16 rr2>
    void LD_rr_rr_m1();
    template<Register16 rr1, Register16 rr2>
    void LD_rr_rr_m2();

    template<Register16 rr1, Register16 rr2>
    void LD_rr_rrn_m1();
    template<Register16 rr1, Register16 rr2>
    void LD_rr_rrn_m2();
    template<Register16 rr1, Register16 rr2>
    void LD_rr_rrn_m3();

    template<Register8 r>
    void INC_r_m1();

    template<Register16 rr>
    void INC_rr_m1();
    template<Register16 rr>
    void INC_rr_m2();

    template<Register16 rr>
    void INC_arr_m1();
    template<Register16 rr>
    void INC_arr_m2();
    template<Register16 rr>
    void INC_arr_m3();

    template<Register8 r>
    void DEC_r_m1();

    template<Register16 rr>
    void DEC_rr_m1();
    template<Register16 rr>
    void DEC_rr_m2();

    template<Register16 rr>
    void DEC_arr_m1();
    template<Register16 rr>
    void DEC_arr_m2();
    template<Register16 rr>
    void DEC_arr_m3();

    template<Register8 r>
    void ADD_r_m1();

    template<Register16 rr>
    void ADD_arr_m1();
    template<Register16 r2>
    void ADD_arr_m2();

    template<Register8 r>
    void ADC_r_m1();

    template<Register16 rr>
    void ADC_arr_m1();
    template<Register16 r2>
    void ADC_arr_m2();

    template<Register16 rr1, Register16 rr2>
    void ADD_rr_rr_m1();
    template<Register16 rr1, Register16 rr2>
    void ADD_rr_rr_m2();

    template<Register16 rr>
    void ADD_rr_n_m1();
    template<Register16 rr>
    void ADD_rr_n_m2();
    template<Register16 rr>
    void ADD_rr_n_m3();
    template<Register16 rr>
    void ADD_rr_n_m4();

    void ADD_n_m1();
    void ADD_n_m2();

    void ADC_n_m1();
    void ADC_n_m2();

    template<Register8 r>
    void SUB_r_m1();

    template<Register16 rr>
    void SUB_arr_m1();
    template<Register16 r2>
    void SUB_arr_m2();

    template<Register8 r>
    void SBC_r_m1();

    template<Register16 rr>
    void SBC_arr_m1();
    template<Register16 r2>
    void SBC_arr_m2();

    void SUB_n_m1();
    void SUB_n_m2();

    void SBC_n_m1();
    void SBC_n_m2();

    template<Register8 r>
    void AND_r_m1();

    template<Register16 rr>
    void AND_arr_m1();
    template<Register16 rr>
    void AND_arr_m2();

    void AND_n_m1();
    void AND_n_m2();

    template<Register8 r>
    void OR_r_m1();

    template<Register16 rr>
    void OR_arr_m1();
    template<Register16 rr>
    void OR_arr_m2();

    void OR_n_m1();
    void OR_n_m2();

    template<Register8 r>
    void XOR_r_m1();

    template<Register16 rr>
    void XOR_arr_m1();
    template<Register16 rr>
    void XOR_arr_m2();

    void XOR_n_m1();
    void XOR_n_m2();

    template<Register8 r>
    void CP_r_m1();

    template<Register16 rr>
    void CP_arr_m1();
    template<Register16 rr>
    void CP_arr_m2();

    void CP_n_m1();
    void CP_n_m2();

    void DAA_m1();
    void SCF_m1();

    void CPL_m1();
    void CCF_m1();

    void RLCA_m1();
    void RLA_m1();

    void RRCA_m1();
    void RRA_m1();

    void JR_n_m1();
    void JR_n_m2();
    void JR_n_m3();

    template<Flag f>
    void JR_c_n_m1();
    template<Flag f>
    void JR_c_n_m2();
    template<Flag f>
    void JR_c_n_m3();

    template<Flag f1, Flag f2>
    void JR_cc_n_m1();
    template<Flag f1, Flag f2>
    void JR_cc_n_m2();
    template<Flag f1, Flag f2>
    void JR_cc_n_m3();

    void JP_nn_m1();
    void JP_nn_m2();
    void JP_nn_m3();
    void JP_nn_m4();

    template<Register16>
    void JP_rr_m1();

    template<Flag f>
    void JP_c_nn_m1();
    template<Flag f>
    void JP_c_nn_m2();
    template<Flag f>
    void JP_c_nn_m3();
    template<Flag f>
    void JP_c_nn_m4();

    template<Flag f1, Flag f2>
    void JP_cc_nn_m1();
    template<Flag f1, Flag f2>
    void JP_cc_nn_m2();
    template<Flag f1, Flag f2>
    void JP_cc_nn_m3();
    template<Flag f1, Flag f2>
    void JP_cc_nn_m4();
};

#endif // CPU_H