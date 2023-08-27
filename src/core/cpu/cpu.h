#ifndef CPU_H
#define CPU_H

#include "core/boot/bootrom.h"
#include "core/clock/clockable.h"
#include "core/state/processor.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

class ICPU : public IClockable {};

class IBus;
class ISerialPort;

class CPU : public ICPU, public IStateProcessor {
public:
    explicit CPU(IBus& bus, IClockable& timers, IClockable& serial, bool bootRom = false);
    ~CPU() override = default;

    void loadState(IReadableState& state) override;
    void saveState(IWritableState& state) override;

    void tick() override;

protected:
    enum class Register8 { A, B, C, D, E, F, H, L, SP_S, SP_P, PC_P, PC_C };

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

    typedef void (CPU::*InstructionMicroOperation)();

    IBus& bus;
    IClockable& timers;
    IClockable& serial;
    bool bootRom;

    uint16_t AF;
    uint16_t BC;
    uint16_t DE;
    uint16_t HL;
    uint16_t PC;
    uint16_t SP;

    bool IME;
    bool pendingEnableIME;

    bool halted;

    uint64_t mCycles;

    // scratchpad
    struct {
        bool b;
        uint8_t u;
        uint8_t u2;
        int8_t s;
        uint16_t uu;
        uint8_t lsb;
        uint8_t msb;
        uint16_t addr;
    };

    struct CurrentInstruction {
        bool ISR;
        uint16_t address;
        uint8_t microop;
        bool cb; // TODO: find another way to save/load from the correct instruction set the current instruction pointer
                 // instead of using this flag
        InstructionMicroOperation* microopSelector;
    } currentInstruction;

    InstructionMicroOperation instructions[256][6];
    InstructionMicroOperation instructions_cb[256][4];
    InstructionMicroOperation ISR[5];

    void reset();

    template <Register8 r>
    [[nodiscard]] uint8_t readRegister8() const;

    template <Register8 r>
    void writeRegister8(uint8_t value);

    template <Register16 rr>
    [[nodiscard]] uint16_t readRegister16() const;

    template <Register16 rr>
    void writeRegister16(uint16_t value);

    template <Flag f>
    [[nodiscard]] bool readFlag() const;
    template <Flag f, bool y = true>
    [[nodiscard]] bool checkFlag() const;

    template <Flag f>
    void writeFlag(bool value);

    [[nodiscard]] bool hasPendingInterrupts() const;
    void serveInterrupt();

    void fetch(bool cb = false);

    /*
     * [INSTRUCTIONS LEGEND]
     * r: 8 bit register
     * rr: 16 bit register
     * rrs: 16 bit register, signed
     * n: 8 bit immediate number
     * nn: 16 bit immediate number
     * s: 8 bit immediate number, signed
     * ar: address specified by 8 bit register
     * arr: address specified by 16 bit register
     * arri: address specified by 16 bit register, then increment 16 bit register
     * an: address specified by 8 bit immediate number
     * ann: address specified by 16 bit immediate number
     * c: check flag condition
     */
    void invalidInstruction();

    void ISR_m1();
    void ISR_m2();
    void ISR_m3();
    void ISR_m4();
    void ISR_m5();

    void NOP_m1();
    void STOP_m1();
    void HALT_m1();
    void DI_m1();
    void EI_m1();

    template <Register16 rr>
    void LD_rr_uu_m1();
    template <Register16 rr>
    void LD_rr_uu_m2();
    template <Register16 rr>
    void LD_rr_uu_m3();

    template <Register16 rr>
    void LD_arr_u_m1();
    template <Register16 rr>
    void LD_arr_u_m2();
    template <Register16 rr>
    void LD_arr_u_m3();

    template <Register16 rr, Register8 r>
    void LD_arr_r_m1();
    template <Register16 rr, Register8 r>
    void LD_arr_r_m2();

    template <Register16 rr, Register8 r, int8_t inc>
    void LD_arri_r_m1();
    template <Register16 rr, Register8 r, int8_t inc>
    void LD_arri_r_m2();

    template <Register8 r>
    void LD_r_u_m1();
    template <Register8 r>
    void LD_r_u_m2();

    template <Register16 rr>
    void LD_ann_rr_m1();
    template <Register16 rr>
    void LD_ann_rr_m2();
    template <Register16 rr>
    void LD_ann_rr_m3();
    template <Register16 rr>
    void LD_ann_rr_m4();
    template <Register16 rr>
    void LD_ann_rr_m5();

    template <Register8 r, Register16 rr>
    void LD_r_arr_m1();
    template <Register8 r, Register16 rr>
    void LD_r_arr_m2();

    template <Register8 r, Register16 rr, int8_t inc>
    void LD_r_arri_m1();
    template <Register8 r, Register16 rr, int8_t inc>
    void LD_r_arri_m2();

    template <Register8 r1, Register8 r2>
    void LD_r_r_m1();

    template <Register8 r>
    void LDH_an_r_m1();
    template <Register8 r>
    void LDH_an_r_m2();
    template <Register8 r>
    void LDH_an_r_m3();

    template <Register8 r>
    void LDH_r_an_m1();
    template <Register8 r>
    void LDH_r_an_m2();
    template <Register8 r>
    void LDH_r_an_m3();

    template <Register8 r1, Register8 r2>
    void LDH_ar_r_m1();
    template <Register8 r1, Register8 r2>
    void LDH_ar_r_m2();

    template <Register8 r1, Register8 r2>
    void LDH_r_ar_m1();
    template <Register8 r1, Register8 r2>
    void LDH_r_ar_m2();

    template <Register8 r>
    void LD_ann_r_m1();
    template <Register8 r>
    void LD_ann_r_m2();
    template <Register8 r>
    void LD_ann_r_m3();
    template <Register8 r>
    void LD_ann_r_m4();

    template <Register8 r>
    void LD_r_ann_m1();
    template <Register8 r>
    void LD_r_ann_m2();
    template <Register8 r>
    void LD_r_ann_m3();
    template <Register8 r>
    void LD_r_ann_m4();

    template <Register16 rr1, Register16 rr2>
    void LD_rr_rr_m1();
    template <Register16 rr1, Register16 rr2>
    void LD_rr_rr_m2();

    template <Register16 rr1, Register16 rr2>
    void LD_rr_rrs_m1();
    template <Register16 rr1, Register16 rr2>
    void LD_rr_rrs_m2();
    template <Register16 rr1, Register16 rr2>
    void LD_rr_rrs_m3();

    template <Register8 r>
    void INC_r_m1();

    template <Register16 rr>
    void INC_rr_m1();
    template <Register16 rr>
    void INC_rr_m2();

    template <Register16 rr>
    void INC_arr_m1();
    template <Register16 rr>
    void INC_arr_m2();
    template <Register16 rr>
    void INC_arr_m3();

    template <Register8 r>
    void DEC_r_m1();

    template <Register16 rr>
    void DEC_rr_m1();
    template <Register16 rr>
    void DEC_rr_m2();

    template <Register16 rr>
    void DEC_arr_m1();
    template <Register16 rr>
    void DEC_arr_m2();
    template <Register16 rr>
    void DEC_arr_m3();

    template <Register8 r>
    void ADD_r_m1();

    template <Register16 rr>
    void ADD_arr_m1();
    template <Register16 r2>
    void ADD_arr_m2();

    template <Register8 r>
    void ADC_r_m1();

    template <Register16 rr>
    void ADC_arr_m1();
    template <Register16 r2>
    void ADC_arr_m2();

    template <Register16 rr1, Register16 rr2>
    void ADD_rr_rr_m1();
    template <Register16 rr1, Register16 rr2>
    void ADD_rr_rr_m2();

    template <Register16 rr>
    void ADD_rr_s_m1();
    template <Register16 rr>
    void ADD_rr_s_m2();
    template <Register16 rr>
    void ADD_rr_s_m3();
    template <Register16 rr>
    void ADD_rr_s_m4();

    void ADD_u_m1();
    void ADD_u_m2();

    void ADC_u_m1();
    void ADC_u_m2();

    template <Register8 r>
    void SUB_r_m1();

    template <Register16 rr>
    void SUB_arr_m1();
    template <Register16 r2>
    void SUB_arr_m2();

    template <Register8 r>
    void SBC_r_m1();

    template <Register16 rr>
    void SBC_arr_m1();
    template <Register16 r2>
    void SBC_arr_m2();

    void SUB_u_m1();
    void SUB_u_m2();

    void SBC_u_m1();
    void SBC_u_m2();

    template <Register8 r>
    void AND_r_m1();

    template <Register16 rr>
    void AND_arr_m1();
    template <Register16 rr>
    void AND_arr_m2();

    void AND_u_m1();
    void AND_u_m2();

    template <Register8 r>
    void OR_r_m1();

    template <Register16 rr>
    void OR_arr_m1();
    template <Register16 rr>
    void OR_arr_m2();

    void OR_u_m1();
    void OR_u_m2();

    template <Register8 r>
    void XOR_r_m1();

    template <Register16 rr>
    void XOR_arr_m1();
    template <Register16 rr>
    void XOR_arr_m2();

    void XOR_u_m1();
    void XOR_u_m2();

    template <Register8 r>
    void CP_r_m1();

    template <Register16 rr>
    void CP_arr_m1();
    template <Register16 rr>
    void CP_arr_m2();

    void CP_u_m1();
    void CP_u_m2();

    void DAA_m1();
    void SCF_m1();

    void CPL_m1();
    void CCF_m1();

    void RLCA_m1();
    void RLA_m1();

    void RRCA_m1();
    void RRA_m1();

    void JR_s_m1();
    void JR_s_m2();
    void JR_s_m3();

    template <Flag f, bool y = true>
    void JR_c_s_m1();
    template <Flag f, bool y = true>
    void JR_c_s_m2();
    template <Flag f, bool y = true>
    void JR_c_s_m3();

    void JP_uu_m1();
    void JP_uu_m2();
    void JP_uu_m3();
    void JP_uu_m4();

    template <Register16>
    void JP_rr_m1();

    template <Flag f, bool y = true>
    void JP_c_uu_m1();
    template <Flag f, bool y = true>
    void JP_c_uu_m2();
    template <Flag f, bool y = true>
    void JP_c_uu_m3();
    template <Flag f, bool y = true>
    void JP_c_uu_m4();

    void CALL_uu_m1();
    void CALL_uu_m2();
    void CALL_uu_m3();
    void CALL_uu_m4();
    void CALL_uu_m5();
    void CALL_uu_m6();

    template <Flag f, bool y = true>
    void CALL_c_uu_m1();
    template <Flag f, bool y = true>
    void CALL_c_uu_m2();
    template <Flag f, bool y = true>
    void CALL_c_uu_m3();
    template <Flag f, bool y = true>
    void CALL_c_uu_m4();
    template <Flag f, bool y = true>
    void CALL_c_uu_m5();
    template <Flag f, bool y = true>
    void CALL_c_uu_m6();

    template <uint8_t n>
    void RST_m1();
    template <uint8_t n>
    void RST_m2();
    template <uint8_t n>
    void RST_m3();
    template <uint8_t n>
    void RST_m4();

    void RET_uu_m1();
    void RET_uu_m2();
    void RET_uu_m3();
    void RET_uu_m4();

    void RETI_uu_m1();
    void RETI_uu_m2();
    void RETI_uu_m3();
    void RETI_uu_m4();

    template <Flag f, bool y = true>
    void RET_c_uu_m1();
    template <Flag f, bool y = true>
    void RET_c_uu_m2();
    template <Flag f, bool y = true>
    void RET_c_uu_m3();
    template <Flag f, bool y = true>
    void RET_c_uu_m4();
    template <Flag f, bool y = true>
    void RET_c_uu_m5();

    template <Register16 rr>
    void PUSH_rr_m1();
    template <Register16 rr>
    void PUSH_rr_m2();
    template <Register16 rr>
    void PUSH_rr_m3();
    template <Register16 rr>
    void PUSH_rr_m4();

    template <Register16 rr>
    void POP_rr_m1();
    template <Register16 rr>
    void POP_rr_m2();
    template <Register16 rr>
    void POP_rr_m3();

    void CB_m1();

    template <Register8 r>
    void RLC_r_m1();

    template <Register16 rr>
    void RLC_arr_m1();
    template <Register16 rr>
    void RLC_arr_m2();
    template <Register16 rr>
    void RLC_arr_m3();

    template <Register8 r>
    void RRC_r_m1();

    template <Register16 rr>
    void RRC_arr_m1();
    template <Register16 rr>
    void RRC_arr_m2();
    template <Register16 rr>
    void RRC_arr_m3();

    template <Register8 r>
    void RL_r_m1();

    template <Register16 rr>
    void RL_arr_m1();
    template <Register16 rr>
    void RL_arr_m2();
    template <Register16 rr>
    void RL_arr_m3();

    template <Register8 r>
    void RR_r_m1();

    template <Register16 rr>
    void RR_arr_m1();
    template <Register16 rr>
    void RR_arr_m2();
    template <Register16 rr>
    void RR_arr_m3();

    template <Register8 r>
    void SLA_r_m1();

    template <Register16 rr>
    void SLA_arr_m1();
    template <Register16 rr>
    void SLA_arr_m2();
    template <Register16 rr>
    void SLA_arr_m3();

    template <Register8 r>
    void SRA_r_m1();

    template <Register16 rr>
    void SRA_arr_m1();
    template <Register16 rr>
    void SRA_arr_m2();
    template <Register16 rr>
    void SRA_arr_m3();

    template <Register8 r>
    void SRL_r_m1();

    template <Register16 rr>
    void SRL_arr_m1();
    template <Register16 rr>
    void SRL_arr_m2();
    template <Register16 rr>
    void SRL_arr_m3();

    template <Register8 r>
    void SWAP_r_m1();

    template <Register16 rr>
    void SWAP_arr_m1();
    template <Register16 rr>
    void SWAP_arr_m2();
    template <Register16 rr>
    void SWAP_arr_m3();

    template <uint8_t n, Register8 r>
    void BIT_r_m1();

    template <uint8_t n, Register16 r>
    void BIT_arr_m1();
    template <uint8_t n, Register16 r>
    void BIT_arr_m2();

    template <uint8_t n, Register8 r>
    void RES_r_m1();

    template <uint8_t n, Register16 r>
    void RES_arr_m1();
    template <uint8_t n, Register16 r>
    void RES_arr_m2();
    template <uint8_t n, Register16 r>
    void RES_arr_m3();

    template <uint8_t n, Register8 r>
    void SET_r_m1();

    template <uint8_t n, Register16 r>
    void SET_arr_m1();
    template <uint8_t n, Register16 r>
    void SET_arr_m2();
    template <uint8_t n, Register16 r>
    void SET_arr_m3();
};

#endif // CPU_H