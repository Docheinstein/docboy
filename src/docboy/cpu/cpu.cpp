#include "cpu.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/mmu/mmu.h"
#include "docboy/serial/port.h"
#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/casts.hpp"
#include "utils/formatters.hpp"

static constexpr uint8_t STATE_INSTRUCTION_FLAG_NORMAL = 0;
static constexpr uint8_t STATE_INSTRUCTION_FLAG_CB = 1;
static constexpr uint8_t STATE_INSTRUCTION_FLAG_ISR = 2;

Cpu::Cpu(InterruptsIO& interrupts, MmuSocket<MmuDevice::Cpu> mmu) :
    interrupts(interrupts),
    mmu(mmu.attach(&busData)),
    // clang-format off
    instructions {
        /* 00 */ { &Cpu::NOP_m0 },
        /* 01 */ { &Cpu::LD_rr_uu_m0<Register16::BC>, &Cpu::LD_rr_uu_m1<Register16::BC>, &Cpu::LD_rr_uu_m2<Register16::BC> },
        /* 02 */ { &Cpu::LD_arr_r_m0<Register16::BC, Register8::A>, &Cpu::LD_arr_r_m1<Register16::BC, Register8::A> },
        /* 03 */ { &Cpu::INC_rr_m0<Register16::BC>, &Cpu::INC_rr_m1<Register16::BC> },
        /* 04 */ { &Cpu::INC_r_m0<Register8::B> },
        /* 05 */ { &Cpu::DEC_r_m0<Register8::B> },
        /* 06 */ { &Cpu::LD_r_u_m0<Register8::B>, &Cpu::LD_r_u_m1<Register8::B> },
        /* 07 */ { &Cpu::RLCA_m0 },
        /* 08 */ { &Cpu::LD_ann_rr_m0<Register16::SP>, &Cpu::LD_ann_rr_m1<Register16::SP>, &Cpu::LD_ann_rr_m2<Register16::SP>, &Cpu::LD_ann_rr_m3<Register16::SP>, &Cpu::LD_ann_rr_m4<Register16::SP> },
        /* 09 */ { &Cpu::ADD_rr_rr_m0<Register16::HL, Register16::BC>, &Cpu::ADD_rr_rr_m1<Register16::HL, Register16::BC> },
        /* 0A */ { &Cpu::LD_r_arr_m0<Register8::A, Register16::BC>, &Cpu::LD_r_arr_m1<Register8::A, Register16::BC> },
        /* 0B */ { &Cpu::DEC_rr_m0<Register16::BC>, &Cpu::DEC_rr_m1<Register16::BC> },
        /* 0C */ { &Cpu::INC_r_m0<Register8::C> },
        /* 0D */ { &Cpu::DEC_r_m0<Register8::C> },
        /* 0E */ { &Cpu::LD_r_u_m0<Register8::C>, &Cpu::LD_r_u_m1<Register8::C> },
        /* 0F */ { &Cpu::RRCA_m0 },
        /* 10 */ { &Cpu::STOP_m0 },
        /* 11 */ { &Cpu::LD_rr_uu_m0<Register16::DE>, &Cpu::LD_rr_uu_m1<Register16::DE>, &Cpu::LD_rr_uu_m2<Register16::DE> },
        /* 12 */ { &Cpu::LD_arr_r_m0<Register16::DE, Register8::A>, &Cpu::LD_arr_r_m1<Register16::DE, Register8::A> },
        /* 13 */ { &Cpu::INC_rr_m0<Register16::DE>, &Cpu::INC_rr_m1<Register16::DE> },
        /* 14 */ { &Cpu::INC_r_m0<Register8::D> },
        /* 15 */ { &Cpu::DEC_r_m0<Register8::D> },
        /* 16 */ { &Cpu::LD_r_u_m0<Register8::D>, &Cpu::LD_r_u_m1<Register8::D> },
        /* 17 */ { &Cpu::RLA_m0 },
        /* 18 */ { &Cpu::JR_s_m0, &Cpu::JR_s_m1, &Cpu::JR_s_m2 },
        /* 19 */ { &Cpu::ADD_rr_rr_m0<Register16::HL, Register16::DE>, &Cpu::ADD_rr_rr_m1<Register16::HL, Register16::DE> },
        /* 1A */ { &Cpu::LD_r_arr_m0<Register8::A, Register16::DE>, &Cpu::LD_r_arr_m1<Register8::A, Register16::DE> },
        /* 1B */ { &Cpu::DEC_rr_m0<Register16::DE>, &Cpu::DEC_rr_m1<Register16::DE> },
        /* 1C */ { &Cpu::INC_r_m0<Register8::E> },
        /* 1D */ { &Cpu::DEC_r_m0<Register8::E> },
        /* 1E */ { &Cpu::LD_r_u_m0<Register8::E>, &Cpu::LD_r_u_m1<Register8::E> },
        /* 1F */ { &Cpu::RRA_m0 },
        /* 20 */ { &Cpu::JR_c_s_m0<Flag::Z, false>, &Cpu::JR_c_s_m1<Flag::Z, false>, &Cpu::JR_c_s_m2<Flag::Z, false> },
        /* 21 */ { &Cpu::LD_rr_uu_m0<Register16::HL>, &Cpu::LD_rr_uu_m1<Register16::HL>, &Cpu::LD_rr_uu_m2<Register16::HL> },
        /* 22 */ { &Cpu::LD_arri_r_m0<Register16::HL, Register8::A, 1>, &Cpu::LD_arri_r_m1<Register16::HL, Register8::A, 1> },
        /* 23 */ { &Cpu::INC_rr_m0<Register16::HL>, &Cpu::INC_rr_m1<Register16::HL> },
        /* 24 */ { &Cpu::INC_r_m0<Register8::H> },
        /* 25 */ { &Cpu::DEC_r_m0<Register8::H> },
        /* 26 */ { &Cpu::LD_r_u_m0<Register8::H>, &Cpu::LD_r_u_m1<Register8::H> },
        /* 27 */ { &Cpu::DAA_m0 },
        /* 28 */ { &Cpu::JR_c_s_m0<Flag::Z>, &Cpu::JR_c_s_m1<Flag::Z>, &Cpu::JR_c_s_m2<Flag::Z> },
        /* 29 */ { &Cpu::ADD_rr_rr_m0<Register16::HL, Register16::HL>, &Cpu::ADD_rr_rr_m1<Register16::HL, Register16::HL> },
        /* 2A */ { &Cpu::LD_r_arri_m0<Register8::A, Register16::HL, 1>, &Cpu::LD_r_arri_m1<Register8::A, Register16::HL, 1> },
        /* 2B */ { &Cpu::DEC_rr_m0<Register16::HL>, &Cpu::DEC_rr_m1<Register16::HL> },
        /* 2C */ { &Cpu::INC_r_m0<Register8::L> },
        /* 2D */ { &Cpu::DEC_r_m0<Register8::L> },
        /* 2E */ { &Cpu::LD_r_u_m0<Register8::L>, &Cpu::LD_r_u_m1<Register8::L> },
        /* 2F */ { &Cpu::CPL_m0 },
        /* 30 */ { &Cpu::JR_c_s_m0<Flag::C, false>, &Cpu::JR_c_s_m1<Flag::C, false>, &Cpu::JR_c_s_m2<Flag::C, false> },
        /* 31 */ { &Cpu::LD_rr_uu_m0<Register16::SP>, &Cpu::LD_rr_uu_m1<Register16::SP>, &Cpu::LD_rr_uu_m2<Register16::SP> },
        /* 32 */ { &Cpu::LD_arri_r_m0<Register16::HL, Register8::A, -1>, &Cpu::LD_arri_r_m1<Register16::HL, Register8::A, -1> },
        /* 33 */ { &Cpu::INC_rr_m0<Register16::SP>, &Cpu::INC_rr_m1<Register16::SP> },
        /* 34 */ { &Cpu::INC_arr_m0<Register16::HL>, &Cpu::INC_arr_m1<Register16::HL>, &Cpu::INC_arr_m2<Register16::HL> },
        /* 35 */ { &Cpu::DEC_arr_m0<Register16::HL>, &Cpu::DEC_arr_m1<Register16::HL>, &Cpu::DEC_arr_m2<Register16::HL> },
        /* 36 */ { &Cpu::LD_arr_u_m0<Register16::HL>, &Cpu::LD_arr_u_m1<Register16::HL>, &Cpu::LD_arr_u_m2<Register16::HL> },
        /* 37 */ { &Cpu::SCF_m0 },
        /* 38 */ { &Cpu::JR_c_s_m0<Flag::C>, &Cpu::JR_c_s_m1<Flag::C>, &Cpu::JR_c_s_m2<Flag::C> },
        /* 39 */ { &Cpu::ADD_rr_rr_m0<Register16::HL, Register16::SP>, &Cpu::ADD_rr_rr_m1<Register16::HL, Register16::SP> },
        /* 3A */ { &Cpu::LD_r_arri_m0<Register8::A, Register16::HL, -1>, &Cpu::LD_r_arri_m1<Register8::A, Register16::HL, -1> },
        /* 3B */ { &Cpu::DEC_rr_m0<Register16::SP>, &Cpu::DEC_rr_m1<Register16::SP> },
        /* 3C */ { &Cpu::INC_r_m0<Register8::A> },
        /* 3D */ { &Cpu::DEC_r_m0<Register8::A> },
        /* 3E */ { &Cpu::LD_r_u_m0<Register8::A>, &Cpu::LD_r_u_m1<Register8::A> },
        /* 3F */ { &Cpu::CCF_m0 },
        /* 40 */ { &Cpu::LD_r_r_m0<Register8::B, Register8::B> },
        /* 41 */ { &Cpu::LD_r_r_m0<Register8::B, Register8::C> },
        /* 42 */ { &Cpu::LD_r_r_m0<Register8::B, Register8::D> },
        /* 43 */ { &Cpu::LD_r_r_m0<Register8::B, Register8::E> },
        /* 44 */ { &Cpu::LD_r_r_m0<Register8::B, Register8::H> },
        /* 45 */ { &Cpu::LD_r_r_m0<Register8::B, Register8::L> },
        /* 46 */ { &Cpu::LD_r_arr_m0<Register8::B, Register16::HL>, &Cpu::LD_r_arr_m1<Register8::B, Register16::HL> },
        /* 47 */ { &Cpu::LD_r_r_m0<Register8::B, Register8::A> },
        /* 48 */ { &Cpu::LD_r_r_m0<Register8::C, Register8::B> },
        /* 49 */ { &Cpu::LD_r_r_m0<Register8::C, Register8::C> },
        /* 4A */ { &Cpu::LD_r_r_m0<Register8::C, Register8::D> },
        /* 4B */ { &Cpu::LD_r_r_m0<Register8::C, Register8::E> },
        /* 4C */ { &Cpu::LD_r_r_m0<Register8::C, Register8::H> },
        /* 4D */ { &Cpu::LD_r_r_m0<Register8::C, Register8::L> },
        /* 4E */ { &Cpu::LD_r_arr_m0<Register8::C, Register16::HL>, &Cpu::LD_r_arr_m1<Register8::C, Register16::HL> },
        /* 4F */ { &Cpu::LD_r_r_m0<Register8::C, Register8::A> },
        /* 50 */ { &Cpu::LD_r_r_m0<Register8::D, Register8::B> },
        /* 51 */ { &Cpu::LD_r_r_m0<Register8::D, Register8::C> },
        /* 52 */ { &Cpu::LD_r_r_m0<Register8::D, Register8::D> },
        /* 53 */ { &Cpu::LD_r_r_m0<Register8::D, Register8::E> },
        /* 54 */ { &Cpu::LD_r_r_m0<Register8::D, Register8::H> },
        /* 55 */ { &Cpu::LD_r_r_m0<Register8::D, Register8::L> },
        /* 56 */ { &Cpu::LD_r_arr_m0<Register8::D, Register16::HL>, &Cpu::LD_r_arr_m1<Register8::D, Register16::HL> },
        /* 57 */ { &Cpu::LD_r_r_m0<Register8::D, Register8::A> },
        /* 58 */ { &Cpu::LD_r_r_m0<Register8::E, Register8::B> },
        /* 59 */ { &Cpu::LD_r_r_m0<Register8::E, Register8::C> },
        /* 5A */ { &Cpu::LD_r_r_m0<Register8::E, Register8::D> },
        /* 5B */ { &Cpu::LD_r_r_m0<Register8::E, Register8::E> },
        /* 5C */ { &Cpu::LD_r_r_m0<Register8::E, Register8::H> },
        /* 5D */ { &Cpu::LD_r_r_m0<Register8::E, Register8::L> },
        /* 5E */ { &Cpu::LD_r_arr_m0<Register8::E, Register16::HL>, &Cpu::LD_r_arr_m1<Register8::E, Register16::HL> },
        /* 5F */ { &Cpu::LD_r_r_m0<Register8::E, Register8::A> },
        /* 60 */ { &Cpu::LD_r_r_m0<Register8::H, Register8::B> },
        /* 61 */ { &Cpu::LD_r_r_m0<Register8::H, Register8::C> },
        /* 62 */ { &Cpu::LD_r_r_m0<Register8::H, Register8::D> },
        /* 63 */ { &Cpu::LD_r_r_m0<Register8::H, Register8::E> },
        /* 64 */ { &Cpu::LD_r_r_m0<Register8::H, Register8::H> },
        /* 65 */ { &Cpu::LD_r_r_m0<Register8::H, Register8::L> },
        /* 66 */ { &Cpu::LD_r_arr_m0<Register8::H, Register16::HL>, &Cpu::LD_r_arr_m1<Register8::H, Register16::HL> },
        /* 67 */ { &Cpu::LD_r_r_m0<Register8::H, Register8::A> },
        /* 68 */ { &Cpu::LD_r_r_m0<Register8::L, Register8::B> },
        /* 69 */ { &Cpu::LD_r_r_m0<Register8::L, Register8::C> },
        /* 6A */ { &Cpu::LD_r_r_m0<Register8::L, Register8::D> },
        /* 6B */ { &Cpu::LD_r_r_m0<Register8::L, Register8::E> },
        /* 6C */ { &Cpu::LD_r_r_m0<Register8::L, Register8::H> },
        /* 6D */ { &Cpu::LD_r_r_m0<Register8::L, Register8::L> },
        /* 6E */ { &Cpu::LD_r_arr_m0<Register8::L, Register16::HL>, &Cpu::LD_r_arr_m1<Register8::L, Register16::HL> },
        /* 6F */ { &Cpu::LD_r_r_m0<Register8::L, Register8::A> },
        /* 70 */ { &Cpu::LD_arr_r_m0<Register16::HL, Register8::B>, &Cpu::LD_arr_r_m1<Register16::HL, Register8::B> },
        /* 71 */ { &Cpu::LD_arr_r_m0<Register16::HL, Register8::C>, &Cpu::LD_arr_r_m1<Register16::HL, Register8::C> },
        /* 72 */ { &Cpu::LD_arr_r_m0<Register16::HL, Register8::D>, &Cpu::LD_arr_r_m1<Register16::HL, Register8::D> },
        /* 73 */ { &Cpu::LD_arr_r_m0<Register16::HL, Register8::E>, &Cpu::LD_arr_r_m1<Register16::HL, Register8::E> },
        /* 74 */ { &Cpu::LD_arr_r_m0<Register16::HL, Register8::H>, &Cpu::LD_arr_r_m1<Register16::HL, Register8::H> },
        /* 75 */ { &Cpu::LD_arr_r_m0<Register16::HL, Register8::L>, &Cpu::LD_arr_r_m1<Register16::HL, Register8::L> },
        /* 76 */ { &Cpu::HALT_m0 },
        /* 77 */ { &Cpu::LD_arr_r_m0<Register16::HL, Register8::A>, &Cpu::LD_arr_r_m1<Register16::HL, Register8::A> },
        /* 78 */ { &Cpu::LD_r_r_m0<Register8::A, Register8::B> },
        /* 79 */ { &Cpu::LD_r_r_m0<Register8::A, Register8::C> },
        /* 7A */ { &Cpu::LD_r_r_m0<Register8::A, Register8::D> },
        /* 7B */ { &Cpu::LD_r_r_m0<Register8::A, Register8::E> },
        /* 7C */ { &Cpu::LD_r_r_m0<Register8::A, Register8::H> },
        /* 7D */ { &Cpu::LD_r_r_m0<Register8::A, Register8::L> },
        /* 7E */ { &Cpu::LD_r_arr_m0<Register8::A, Register16::HL>, &Cpu::LD_r_arr_m1<Register8::A, Register16::HL> },
        /* 7F */ { &Cpu::LD_r_r_m0<Register8::A, Register8::A> },
        /* 80 */ { &Cpu::ADD_r_m0<Register8::B> },
        /* 81 */ { &Cpu::ADD_r_m0<Register8::C> },
        /* 82 */ { &Cpu::ADD_r_m0<Register8::D> },
        /* 83 */ { &Cpu::ADD_r_m0<Register8::E> },
        /* 84 */ { &Cpu::ADD_r_m0<Register8::H> },
        /* 85 */ { &Cpu::ADD_r_m0<Register8::L> },
        /* 86 */ { &Cpu::ADD_arr_m0<Register16::HL>, &Cpu::ADD_arr_m1<Register16::HL> },
        /* 87 */ { &Cpu::ADD_r_m0<Register8::A> },
        /* 88 */ { &Cpu::ADC_r_m0<Register8::B> },
        /* 89 */ { &Cpu::ADC_r_m0<Register8::C> },
        /* 8A */ { &Cpu::ADC_r_m0<Register8::D> },
        /* 8B */ { &Cpu::ADC_r_m0<Register8::E> },
        /* 8C */ { &Cpu::ADC_r_m0<Register8::H> },
        /* 8D */ { &Cpu::ADC_r_m0<Register8::L> },
        /* 8E */ { &Cpu::ADC_arr_m0<Register16::HL>, &Cpu::ADC_arr_m1<Register16::HL> },
        /* 8F */ { &Cpu::ADC_r_m0<Register8::A> },
        /* 90 */ { &Cpu::SUB_r_m0<Register8::B> },
        /* 91 */ { &Cpu::SUB_r_m0<Register8::C> },
        /* 92 */ { &Cpu::SUB_r_m0<Register8::D> },
        /* 93 */ { &Cpu::SUB_r_m0<Register8::E> },
        /* 94 */ { &Cpu::SUB_r_m0<Register8::H> },
        /* 95 */ { &Cpu::SUB_r_m0<Register8::L> },
        /* 96 */ { &Cpu::SUB_arr_m0<Register16::HL>, &Cpu::SUB_arr_m1<Register16::HL> },
        /* 97 */ { &Cpu::SUB_r_m0<Register8::A> },
        /* 98 */ { &Cpu::SBC_r_m0<Register8::B> },
        /* 99 */ { &Cpu::SBC_r_m0<Register8::C> },
        /* 9A */ { &Cpu::SBC_r_m0<Register8::D> },
        /* 9B */ { &Cpu::SBC_r_m0<Register8::E> },
        /* 9C */ { &Cpu::SBC_r_m0<Register8::H> },
        /* 9D */ { &Cpu::SBC_r_m0<Register8::L> },
        /* 9E */ { &Cpu::SBC_arr_m0<Register16::HL>, &Cpu::SBC_arr_m1<Register16::HL>  },
        /* 9F */ { &Cpu::SBC_r_m0<Register8::A> },
        /* A0 */ { &Cpu::AND_r_m0<Register8::B> },
        /* A1 */ { &Cpu::AND_r_m0<Register8::C> },
        /* A2 */ { &Cpu::AND_r_m0<Register8::D> },
        /* A3 */ { &Cpu::AND_r_m0<Register8::E> },
        /* A4 */ { &Cpu::AND_r_m0<Register8::H> },
        /* A5 */ { &Cpu::AND_r_m0<Register8::L> },
        /* A6 */ { &Cpu::AND_arr_m0<Register16::HL>, &Cpu::AND_arr_m1<Register16::HL> },
        /* A7 */ { &Cpu::AND_r_m0<Register8::A> },
        /* A8 */ { &Cpu::XOR_r_m0<Register8::B> },
        /* A9 */ { &Cpu::XOR_r_m0<Register8::C> },
        /* AA */ { &Cpu::XOR_r_m0<Register8::D> },
        /* AB */ { &Cpu::XOR_r_m0<Register8::E>  },
        /* AC */ { &Cpu::XOR_r_m0<Register8::H>  },
        /* AD */ { &Cpu::XOR_r_m0<Register8::L>  },
        /* AE */ { &Cpu::XOR_arr_m0<Register16::HL>, &Cpu::XOR_arr_m1<Register16::HL> },
        /* AF */ { &Cpu::XOR_r_m0<Register8::A> },
        /* B0 */ { &Cpu::OR_r_m0<Register8::B> },
        /* B1 */ { &Cpu::OR_r_m0<Register8::C> },
        /* B2 */ { &Cpu::OR_r_m0<Register8::D> },
        /* B3 */ { &Cpu::OR_r_m0<Register8::E> },
        /* B4 */ { &Cpu::OR_r_m0<Register8::H> },
        /* B5 */ { &Cpu::OR_r_m0<Register8::L> },
        /* B6 */ { &Cpu::OR_arr_m0<Register16::HL>, &Cpu::OR_arr_m1<Register16::HL> },
        /* B7 */ { &Cpu::OR_r_m0<Register8::A> },
        /* B8 */ { &Cpu::CP_r_m0<Register8::B> },
        /* B9 */ { &Cpu::CP_r_m0<Register8::C> },
        /* BA */ { &Cpu::CP_r_m0<Register8::D> },
        /* BB */ { &Cpu::CP_r_m0<Register8::E> },
        /* BC */ { &Cpu::CP_r_m0<Register8::H> },
        /* BD */ { &Cpu::CP_r_m0<Register8::L> },
        /* BE */ { &Cpu::CP_arr_m0<Register16::HL>, &Cpu::CP_arr_m1<Register16::HL> },
        /* BF */ { &Cpu::CP_r_m0<Register8::A> },
        /* C0 */ { &Cpu::RET_c_uu_m0<Flag::Z, false>, &Cpu::RET_c_uu_m1<Flag::Z, false>, &Cpu::RET_c_uu_m2<Flag::Z, false>,
                    &Cpu::RET_c_uu_m3<Flag::Z, false>, &Cpu::RET_c_uu_m4<Flag::Z, false> },
        /* C1 */ { &Cpu::POP_rr_m0<Register16::BC>, &Cpu::POP_rr_m1<Register16::BC>, &Cpu::POP_rr_m2<Register16::BC> },
        /* C2 */ { &Cpu::JP_c_uu_m0<Flag::Z, false>, &Cpu::JP_c_uu_m1<Flag::Z, false>, &Cpu::JP_c_uu_m2<Flag::Z, false>, &Cpu::JP_c_uu_m3<Flag::Z, false> },
        /* C3 */ { &Cpu::JP_uu_m0, &Cpu::JP_uu_m1, &Cpu::JP_uu_m2, &Cpu::JP_uu_m3 },
        /* C4 */ { &Cpu::CALL_c_uu_m0<Flag::Z, false>, &Cpu::CALL_c_uu_m1<Flag::Z, false>, &Cpu::CALL_c_uu_m2<Flag::Z, false>,
                    &Cpu::CALL_c_uu_m3<Flag::Z, false>, &Cpu::CALL_c_uu_m4<Flag::Z, false>, &Cpu::CALL_c_uu_m5<Flag::Z, false>, },
        /* C5 */ { &Cpu::PUSH_rr_m0<Register16::BC>, &Cpu::PUSH_rr_m1<Register16::BC>, &Cpu::PUSH_rr_m2<Register16::BC>, &Cpu::PUSH_rr_m3<Register16::BC> },
        /* C6 */ { &Cpu::ADD_u_m0, &Cpu::ADD_u_m1 },
        /* C7 */ { &Cpu::RST_m0<0x00>, &Cpu::RST_m1<0x00>, &Cpu::RST_m2<0x00>, &Cpu::RST_m3<0x00> },
        /* C8 */ { &Cpu::RET_c_uu_m0<Flag::Z>, &Cpu::RET_c_uu_m1<Flag::Z>, &Cpu::RET_c_uu_m2<Flag::Z>,
                    &Cpu::RET_c_uu_m3<Flag::Z>, &Cpu::RET_c_uu_m4<Flag::Z> },
        /* C9 */ { &Cpu::RET_uu_m0, &Cpu::RET_uu_m1, &Cpu::RET_uu_m2, &Cpu::RET_uu_m3 },
        /* CA */ { &Cpu::JP_c_uu_m0<Flag::Z>, &Cpu::JP_c_uu_m1<Flag::Z>, &Cpu::JP_c_uu_m2<Flag::Z>, &Cpu::JP_c_uu_m3<Flag::Z> },
        /* CB */ { &Cpu::CB_m0 },
        /* CC */ { &Cpu::CALL_c_uu_m0<Flag::Z>, &Cpu::CALL_c_uu_m1<Flag::Z>, &Cpu::CALL_c_uu_m2<Flag::Z>,
                    &Cpu::CALL_c_uu_m3<Flag::Z>, &Cpu::CALL_c_uu_m4<Flag::Z>, &Cpu::CALL_c_uu_m5<Flag::Z>, },
        /* CD */ { &Cpu::CALL_uu_m0, &Cpu::CALL_uu_m1, &Cpu::CALL_uu_m2,
                    &Cpu::CALL_uu_m3, &Cpu::CALL_uu_m4, &Cpu::CALL_uu_m5, },
        /* CE */ { &Cpu::ADC_u_m0, &Cpu::ADC_u_m1 },
        /* CF */ { &Cpu::RST_m0<0x08>, &Cpu::RST_m1<0x08>, &Cpu::RST_m2<0x08>, &Cpu::RST_m3<0x08> },
        /* D0 */ { &Cpu::RET_c_uu_m0<Flag::C, false>, &Cpu::RET_c_uu_m1<Flag::C, false>, &Cpu::RET_c_uu_m2<Flag::C, false>,
                    &Cpu::RET_c_uu_m3<Flag::C, false>, &Cpu::RET_c_uu_m4<Flag::C, false> },
        /* D1 */ { &Cpu::POP_rr_m0<Register16::DE>, &Cpu::POP_rr_m1<Register16::DE>, &Cpu::POP_rr_m2<Register16::DE> },
        /* D2 */ { &Cpu::JP_c_uu_m0<Flag::C, false>, &Cpu::JP_c_uu_m1<Flag::C, false>, &Cpu::JP_c_uu_m2<Flag::C, false>, &Cpu::JP_c_uu_m3<Flag::C, false> },
        /* D3 */ { &Cpu::invalidMicroOperation },
        /* D4 */ { &Cpu::CALL_c_uu_m0<Flag::C, false>, &Cpu::CALL_c_uu_m1<Flag::C, false>, &Cpu::CALL_c_uu_m2<Flag::C, false>,
                    &Cpu::CALL_c_uu_m3<Flag::C, false>, &Cpu::CALL_c_uu_m4<Flag::C, false>, &Cpu::CALL_c_uu_m5<Flag::C, false>, },
        /* D5 */ { &Cpu::PUSH_rr_m0<Register16::DE>, &Cpu::PUSH_rr_m1<Register16::DE>, &Cpu::PUSH_rr_m2<Register16::DE>, &Cpu::PUSH_rr_m3<Register16::DE> },
        /* D6 */ { &Cpu::SUB_u_m0, &Cpu::SUB_u_m1 },
        /* D7 */ { &Cpu::RST_m0<0x10>, &Cpu::RST_m1<0x10>, &Cpu::RST_m2<0x10>, &Cpu::RST_m3<0x10> },
        /* D8 */ { &Cpu::RET_c_uu_m0<Flag::C>, &Cpu::RET_c_uu_m1<Flag::C>, &Cpu::RET_c_uu_m2<Flag::C>,
                    &Cpu::RET_c_uu_m3<Flag::C>, &Cpu::RET_c_uu_m4<Flag::C> },
        /* D9 */ { &Cpu::RETI_uu_m0, &Cpu::RETI_uu_m1, &Cpu::RETI_uu_m2, &Cpu::RETI_uu_m3 },
        /* DA */ { &Cpu::JP_c_uu_m0<Flag::C>, &Cpu::JP_c_uu_m1<Flag::C>, &Cpu::JP_c_uu_m2<Flag::C>, &Cpu::JP_c_uu_m3<Flag::C> },
        /* DB */ { &Cpu::invalidMicroOperation },
        /* DC */ { &Cpu::CALL_c_uu_m0<Flag::C>, &Cpu::CALL_c_uu_m1<Flag::C>, &Cpu::CALL_c_uu_m2<Flag::C>,
                    &Cpu::CALL_c_uu_m3<Flag::C>, &Cpu::CALL_c_uu_m4<Flag::C>, &Cpu::CALL_c_uu_m5<Flag::C>, },
        /* DD */ { &Cpu::invalidMicroOperation },
        /* DE */ { &Cpu::SBC_u_m0, &Cpu::SBC_u_m1 },
        /* DF */ { &Cpu::RST_m0<0x18>, &Cpu::RST_m1<0x18>, &Cpu::RST_m2<0x18>, &Cpu::RST_m3<0x18> },
        /* E0 */ { &Cpu::LDH_an_r_m0<Register8::A>, &Cpu::LDH_an_r_m1<Register8::A>, &Cpu::LDH_an_r_m2<Register8::A> },
        /* E1 */ { &Cpu::POP_rr_m0<Register16::HL>, &Cpu::POP_rr_m1<Register16::HL>, &Cpu::POP_rr_m2<Register16::HL> },
        /* E2 */ { &Cpu::LDH_ar_r_m0<Register8::C, Register8::A>, &Cpu::LDH_ar_r_m1<Register8::C, Register8::A> },
        /* E3 */ { &Cpu::invalidMicroOperation },
        /* E4 */ { &Cpu::invalidMicroOperation },
        /* E5 */ { &Cpu::PUSH_rr_m0<Register16::HL>, &Cpu::PUSH_rr_m1<Register16::HL>, &Cpu::PUSH_rr_m2<Register16::HL>, &Cpu::PUSH_rr_m3<Register16::HL> },
        /* E6 */ { &Cpu::AND_u_m0, &Cpu::AND_u_m1 },
        /* E7 */ { &Cpu::RST_m0<0x20>, &Cpu::RST_m1<0x20>, &Cpu::RST_m2<0x20>, &Cpu::RST_m3<0x20> },
        /* E8 */ { &Cpu::ADD_rr_s_m0<Register16::SP>, &Cpu::ADD_rr_s_m1<Register16::SP>, &Cpu::ADD_rr_s_m2<Register16::SP>, &Cpu::ADD_rr_s_m3<Register16::SP> },
        /* E9 */ { &Cpu::JP_rr_m0<Register16::HL> },
        /* EA */ { &Cpu::LD_ann_r_m0<Register8::A>, &Cpu::LD_ann_r_m1<Register8::A>, &Cpu::LD_ann_r_m2<Register8::A>, &Cpu::LD_ann_r_m3<Register8::A> },
        /* EB */ { &Cpu::invalidMicroOperation },
        /* EC */ { &Cpu::invalidMicroOperation },
        /* ED */ { &Cpu::invalidMicroOperation },
        /* EE */ { &Cpu::XOR_u_m0, &Cpu::XOR_u_m1 },
        /* EF */ { &Cpu::RST_m0<0x28>, &Cpu::RST_m1<0x28>, &Cpu::RST_m2<0x28>, &Cpu::RST_m3<0x28> },
        /* F0 */ { &Cpu::LDH_r_an_m0<Register8::A>, &Cpu::LDH_r_an_m1<Register8::A>, &Cpu::LDH_r_an_m2<Register8::A> },
        /* F1 */ { &Cpu::POP_rr_m0<Register16::AF>, &Cpu::POP_rr_m1<Register16::AF>, &Cpu::POP_rr_m2<Register16::AF> },
        /* F2 */ { &Cpu::LDH_r_ar_m0<Register8::A, Register8::C>, &Cpu::LDH_r_ar_m1<Register8::A, Register8::C> },
        /* F3 */ { &Cpu::DI_m0 },
        /* F4 */ { &Cpu::invalidMicroOperation },
        /* F5 */ { &Cpu::PUSH_rr_m0<Register16::AF>, &Cpu::PUSH_rr_m1<Register16::AF>, &Cpu::PUSH_rr_m2<Register16::AF>, &Cpu::PUSH_rr_m3<Register16::AF> },
        /* F6 */ { &Cpu::OR_u_m0, &Cpu::OR_u_m1 },
        /* F7 */ { &Cpu::RST_m0<0x30>, &Cpu::RST_m1<0x30>, &Cpu::RST_m2<0x30>, &Cpu::RST_m3<0x30> },
        /* F8 */ { &Cpu::LD_rr_rrs_m0<Register16::HL, Register16::SP>, &Cpu::LD_rr_rrs_m1<Register16::HL, Register16::SP>, &Cpu::LD_rr_rrs_m2<Register16::HL, Register16::SP> },
        /* F9 */ { &Cpu::LD_rr_rr_m0<Register16::SP, Register16::HL>, &Cpu::LD_rr_rr_m1<Register16::SP, Register16::HL> },
        /* FA */ { &Cpu::LD_r_ann_m0<Register8::A>, &Cpu::LD_r_ann_m1<Register8::A>, &Cpu::LD_r_ann_m2<Register8::A>, &Cpu::LD_r_ann_m3<Register8::A> },
        /* FB */ { &Cpu::EI_m0 },
        /* FC */ { &Cpu::invalidMicroOperation },
        /* FD */ { &Cpu::invalidMicroOperation },
        /* FE */ { &Cpu::CP_u_m0, &Cpu::CP_u_m1 },
        /* FF */ { &Cpu::RST_m0<0x38>, &Cpu::RST_m1<0x38>, &Cpu::RST_m2<0x38>, &Cpu::RST_m3<0x38> },
    },
    instructionsCB {
        /* 00 */ { &Cpu::RLC_r_m0<Register8::B> },
        /* 01 */ { &Cpu::RLC_r_m0<Register8::C> },
        /* 02 */ { &Cpu::RLC_r_m0<Register8::D> },
        /* 03 */ { &Cpu::RLC_r_m0<Register8::E> },
        /* 04 */ { &Cpu::RLC_r_m0<Register8::H> },
        /* 05 */ { &Cpu::RLC_r_m0<Register8::L> },
        /* 06 */ { &Cpu::RLC_arr_m0<Register16::HL>,    &Cpu::RLC_arr_m1<Register16::HL>,    &Cpu::RLC_arr_m2<Register16::HL> },
        /* 07 */ { &Cpu::RLC_r_m0<Register8::A> },
        /* 08 */ { &Cpu::RRC_r_m0<Register8::B> },
        /* 09 */ { &Cpu::RRC_r_m0<Register8::C> },
        /* 0A */ { &Cpu::RRC_r_m0<Register8::D> },
        /* 0B */ { &Cpu::RRC_r_m0<Register8::E> },
        /* 0C */ { &Cpu::RRC_r_m0<Register8::H> },
        /* 0D */ { &Cpu::RRC_r_m0<Register8::L> },
        /* 0E */ { &Cpu::RRC_arr_m0<Register16::HL>,    &Cpu::RRC_arr_m1<Register16::HL>,    &Cpu::RRC_arr_m2<Register16::HL> },
        /* 0F */ { &Cpu::RRC_r_m0<Register8::A> },
        /* 10 */ { &Cpu::RL_r_m0<Register8::B> },
        /* 11 */ { &Cpu::RL_r_m0<Register8::C> },
        /* 12 */ { &Cpu::RL_r_m0<Register8::D> },
        /* 13 */ { &Cpu::RL_r_m0<Register8::E> },
        /* 14 */ { &Cpu::RL_r_m0<Register8::H> },
        /* 15 */ { &Cpu::RL_r_m0<Register8::L> },
        /* 16 */ { &Cpu::RL_arr_m0<Register16::HL>,     &Cpu::RL_arr_m1<Register16::HL>,     &Cpu::RL_arr_m2<Register16::HL> },
        /* 17 */ { &Cpu::RL_r_m0<Register8::A> },
        /* 18 */ { &Cpu::RR_r_m0<Register8::B> },
        /* 19 */ { &Cpu::RR_r_m0<Register8::C> },
        /* 1A */ { &Cpu::RR_r_m0<Register8::D> },
        /* 1B */ { &Cpu::RR_r_m0<Register8::E> },
        /* 1C */ { &Cpu::RR_r_m0<Register8::H> },
        /* 1D */ { &Cpu::RR_r_m0<Register8::L> },
        /* 1E */ { &Cpu::RR_arr_m0<Register16::HL>,     &Cpu::RR_arr_m1<Register16::HL>,     &Cpu::RR_arr_m2<Register16::HL> },
        /* 1F */ { &Cpu::RR_r_m0<Register8::A> },
        /* 20 */ { &Cpu::SLA_r_m0<Register8::B> },
        /* 21 */ { &Cpu::SLA_r_m0<Register8::C> },
        /* 22 */ { &Cpu::SLA_r_m0<Register8::D> },
        /* 23 */ { &Cpu::SLA_r_m0<Register8::E> },
        /* 24 */ { &Cpu::SLA_r_m0<Register8::H> },
        /* 25 */ { &Cpu::SLA_r_m0<Register8::L> },
        /* 26 */ { &Cpu::SLA_arr_m0<Register16::HL>,    &Cpu::SLA_arr_m1<Register16::HL>,    &Cpu::SLA_arr_m2<Register16::HL>  },
        /* 27 */ { &Cpu::SLA_r_m0<Register8::A> },
        /* 28 */ { &Cpu::SRA_r_m0<Register8::B> },
        /* 29 */ { &Cpu::SRA_r_m0<Register8::C> },
        /* 2A */ { &Cpu::SRA_r_m0<Register8::D> },
        /* 2B */ { &Cpu::SRA_r_m0<Register8::E> },
        /* 2C */ { &Cpu::SRA_r_m0<Register8::H> },
        /* 2D */ { &Cpu::SRA_r_m0<Register8::L> },
        /* 2E */ { &Cpu::SRA_arr_m0<Register16::HL>,    &Cpu::SRA_arr_m1<Register16::HL>,    &Cpu::SRA_arr_m2<Register16::HL> },
        /* 2F */ { &Cpu::SRA_r_m0<Register8::A> },
        /* 30 */ { &Cpu::SWAP_r_m0<Register8::B> },
        /* 31 */ { &Cpu::SWAP_r_m0<Register8::C> },
        /* 32 */ { &Cpu::SWAP_r_m0<Register8::D> },
        /* 33 */ { &Cpu::SWAP_r_m0<Register8::E> },
        /* 34 */ { &Cpu::SWAP_r_m0<Register8::H> },
        /* 35 */ { &Cpu::SWAP_r_m0<Register8::L> },
        /* 36 */ { &Cpu::SWAP_arr_m0<Register16::HL>,   &Cpu::SWAP_arr_m1<Register16::HL>,   &Cpu::SWAP_arr_m2<Register16::HL> },
        /* 37 */ { &Cpu::SWAP_r_m0<Register8::A> },
        /* 38 */ { &Cpu::SRL_r_m0<Register8::B> },
        /* 39 */ { &Cpu::SRL_r_m0<Register8::C> },
        /* 3A */ { &Cpu::SRL_r_m0<Register8::D> },
        /* 3B */ { &Cpu::SRL_r_m0<Register8::E> },
        /* 3C */ { &Cpu::SRL_r_m0<Register8::H> },
        /* 3D */ { &Cpu::SRL_r_m0<Register8::L> },
        /* 3E */ { &Cpu::SRL_arr_m0<Register16::HL>,    &Cpu::SRL_arr_m1<Register16::HL>,    &Cpu::SRL_arr_m2<Register16::HL> },
        /* 3F */ { &Cpu::SRL_r_m0<Register8::A> },
        /* 40 */ { &Cpu::BIT_r_m0<0, Register8::B> },
        /* 41 */ { &Cpu::BIT_r_m0<0, Register8::C> },
        /* 42 */ { &Cpu::BIT_r_m0<0, Register8::D> },
        /* 43 */ { &Cpu::BIT_r_m0<0, Register8::E> },
        /* 44 */ { &Cpu::BIT_r_m0<0, Register8::H> },
        /* 45 */ { &Cpu::BIT_r_m0<0, Register8::L> },
        /* 46 */ { &Cpu::BIT_arr_m0<0, Register16::HL>, &Cpu::BIT_arr_m1<0, Register16::HL> },
        /* 47 */ { &Cpu::BIT_r_m0<0, Register8::A> },
        /* 48 */ { &Cpu::BIT_r_m0<1, Register8::B> },
        /* 49 */ { &Cpu::BIT_r_m0<1, Register8::C> },
        /* 4A */ { &Cpu::BIT_r_m0<1, Register8::D> },
        /* 4B */ { &Cpu::BIT_r_m0<1, Register8::E> },
        /* 4C */ { &Cpu::BIT_r_m0<1, Register8::H> },
        /* 4D */ { &Cpu::BIT_r_m0<1, Register8::L> },
        /* 4E */ { &Cpu::BIT_arr_m0<1, Register16::HL>, &Cpu::BIT_arr_m1<1, Register16::HL> },
        /* 4F */ { &Cpu::BIT_r_m0<1, Register8::A> },
        /* 50 */ { &Cpu::BIT_r_m0<2, Register8::B> },
        /* 51 */ { &Cpu::BIT_r_m0<2, Register8::C> },
        /* 52 */ { &Cpu::BIT_r_m0<2, Register8::D> },
        /* 53 */ { &Cpu::BIT_r_m0<2, Register8::E> },
        /* 54 */ { &Cpu::BIT_r_m0<2, Register8::H> },
        /* 55 */ { &Cpu::BIT_r_m0<2, Register8::L> },
        /* 56 */ { &Cpu::BIT_arr_m0<2, Register16::HL>, &Cpu::BIT_arr_m1<2, Register16::HL> },
        /* 57 */ { &Cpu::BIT_r_m0<2, Register8::A> },
        /* 58 */ { &Cpu::BIT_r_m0<3, Register8::B> },
        /* 59 */ { &Cpu::BIT_r_m0<3, Register8::C> },
        /* 5A */ { &Cpu::BIT_r_m0<3, Register8::D> },
        /* 5B */ { &Cpu::BIT_r_m0<3, Register8::E> },
        /* 5C */ { &Cpu::BIT_r_m0<3, Register8::H> },
        /* 5D */ { &Cpu::BIT_r_m0<3, Register8::L> },
        /* 5E */ { &Cpu::BIT_arr_m0<3, Register16::HL>, &Cpu::BIT_arr_m1<3, Register16::HL> },
        /* 5F */ { &Cpu::BIT_r_m0<3, Register8::A> },
        /* 60 */ { &Cpu::BIT_r_m0<4, Register8::B> },
        /* 61 */ { &Cpu::BIT_r_m0<4, Register8::C> },
        /* 62 */ { &Cpu::BIT_r_m0<4, Register8::D> },
        /* 63 */ { &Cpu::BIT_r_m0<4, Register8::E> },
        /* 64 */ { &Cpu::BIT_r_m0<4, Register8::H> },
        /* 65 */ { &Cpu::BIT_r_m0<4, Register8::L> },
        /* 66 */ { &Cpu::BIT_arr_m0<4, Register16::HL>, &Cpu::BIT_arr_m1<4, Register16::HL> },
        /* 67 */ { &Cpu::BIT_r_m0<4, Register8::A> },
        /* 68 */ { &Cpu::BIT_r_m0<5, Register8::B> },
        /* 69 */ { &Cpu::BIT_r_m0<5, Register8::C> },
        /* 6A */ { &Cpu::BIT_r_m0<5, Register8::D> },
        /* 6B */ { &Cpu::BIT_r_m0<5, Register8::E> },
        /* 6C */ { &Cpu::BIT_r_m0<5, Register8::H> },
        /* 6D */ { &Cpu::BIT_r_m0<5, Register8::L> },
        /* 6E */ { &Cpu::BIT_arr_m0<5, Register16::HL>, &Cpu::BIT_arr_m1<5, Register16::HL> },
        /* 6F */ { &Cpu::BIT_r_m0<5, Register8::A> },
        /* 70 */ { &Cpu::BIT_r_m0<6, Register8::B> },
        /* 71 */ { &Cpu::BIT_r_m0<6, Register8::C> },
        /* 72 */ { &Cpu::BIT_r_m0<6, Register8::D> },
        /* 73 */ { &Cpu::BIT_r_m0<6, Register8::E> },
        /* 74 */ { &Cpu::BIT_r_m0<6, Register8::H> },
        /* 75 */ { &Cpu::BIT_r_m0<6, Register8::L> },
        /* 76 */ { &Cpu::BIT_arr_m0<6, Register16::HL>, &Cpu::BIT_arr_m1<6, Register16::HL> },
        /* 77 */ { &Cpu::BIT_r_m0<6, Register8::A> },
        /* 78 */ { &Cpu::BIT_r_m0<7, Register8::B> },
        /* 79 */ { &Cpu::BIT_r_m0<7, Register8::C> },
        /* 7A */ { &Cpu::BIT_r_m0<7, Register8::D> },
        /* 7B */ { &Cpu::BIT_r_m0<7, Register8::E> },
        /* 7C */ { &Cpu::BIT_r_m0<7, Register8::H> },
        /* 7D */ { &Cpu::BIT_r_m0<7, Register8::L> },
        /* 7E */ { &Cpu::BIT_arr_m0<7, Register16::HL>, &Cpu::BIT_arr_m1<7, Register16::HL> },
        /* 7F */ { &Cpu::BIT_r_m0<7, Register8::A> },
        /* 80 */ { &Cpu::RES_r_m0<0, Register8::B> },
        /* 81 */ { &Cpu::RES_r_m0<0, Register8::C> },
        /* 82 */ { &Cpu::RES_r_m0<0, Register8::D> },
        /* 83 */ { &Cpu::RES_r_m0<0, Register8::E> },
        /* 84 */ { &Cpu::RES_r_m0<0, Register8::H> },
        /* 85 */ { &Cpu::RES_r_m0<0, Register8::L> },
        /* 86 */ { &Cpu::RES_arr_m0<0, Register16::HL>, &Cpu::RES_arr_m1<0, Register16::HL>, &Cpu::RES_arr_m2<0, Register16::HL> },
        /* 87 */ { &Cpu::RES_r_m0<0, Register8::A> },
        /* 88 */ { &Cpu::RES_r_m0<1, Register8::B> },
        /* 89 */ { &Cpu::RES_r_m0<1, Register8::C> },
        /* 8A */ { &Cpu::RES_r_m0<1, Register8::D> },
        /* 8B */ { &Cpu::RES_r_m0<1, Register8::E> },
        /* 8C */ { &Cpu::RES_r_m0<1, Register8::H> },
        /* 8D */ { &Cpu::RES_r_m0<1, Register8::L> },
        /* 8E */ { &Cpu::RES_arr_m0<1, Register16::HL>, &Cpu::RES_arr_m1<1, Register16::HL>, &Cpu::RES_arr_m2<1, Register16::HL> },
        /* 8F */ { &Cpu::RES_r_m0<1, Register8::A> },
        /* 90 */ { &Cpu::RES_r_m0<2, Register8::B> },
        /* 91 */ { &Cpu::RES_r_m0<2, Register8::C> },
        /* 92 */ { &Cpu::RES_r_m0<2, Register8::D> },
        /* 93 */ { &Cpu::RES_r_m0<2, Register8::E> },
        /* 94 */ { &Cpu::RES_r_m0<2, Register8::H> },
        /* 95 */ { &Cpu::RES_r_m0<2, Register8::L> },
        /* 96 */ { &Cpu::RES_arr_m0<2, Register16::HL>, &Cpu::RES_arr_m1<2, Register16::HL>, &Cpu::RES_arr_m2<2, Register16::HL> },
        /* 97 */ { &Cpu::RES_r_m0<2, Register8::A> },
        /* 98 */ { &Cpu::RES_r_m0<3, Register8::B> },
        /* 99 */ { &Cpu::RES_r_m0<3, Register8::C> },
        /* 9A */ { &Cpu::RES_r_m0<3, Register8::D> },
        /* 9B */ { &Cpu::RES_r_m0<3, Register8::E> },
        /* 9C */ { &Cpu::RES_r_m0<3, Register8::H> },
        /* 9D */ { &Cpu::RES_r_m0<3, Register8::L> },
        /* 9E */ { &Cpu::RES_arr_m0<3, Register16::HL>, &Cpu::RES_arr_m1<3, Register16::HL>, &Cpu::RES_arr_m2<3, Register16::HL> },
        /* 9F */ { &Cpu::RES_r_m0<3, Register8::A> },
        /* A0 */ { &Cpu::RES_r_m0<4, Register8::B> },
        /* A1 */ { &Cpu::RES_r_m0<4, Register8::C> },
        /* A2 */ { &Cpu::RES_r_m0<4, Register8::D> },
        /* A3 */ { &Cpu::RES_r_m0<4, Register8::E> },
        /* A4 */ { &Cpu::RES_r_m0<4, Register8::H> },
        /* A5 */ { &Cpu::RES_r_m0<4, Register8::L> },
        /* A6 */ { &Cpu::RES_arr_m0<4, Register16::HL>, &Cpu::RES_arr_m1<4, Register16::HL>, &Cpu::RES_arr_m2<4, Register16::HL> },
        /* A7 */ { &Cpu::RES_r_m0<4, Register8::A> },
        /* A8 */ { &Cpu::RES_r_m0<5, Register8::B> },
        /* A9 */ { &Cpu::RES_r_m0<5, Register8::C> },
        /* AA */ { &Cpu::RES_r_m0<5, Register8::D> },
        /* AB */ { &Cpu::RES_r_m0<5, Register8::E> },
        /* AC */ { &Cpu::RES_r_m0<5, Register8::H> },
        /* AD */ { &Cpu::RES_r_m0<5, Register8::L> },
        /* AE */ { &Cpu::RES_arr_m0<5, Register16::HL>, &Cpu::RES_arr_m1<5, Register16::HL>, &Cpu::RES_arr_m2<5, Register16::HL> },
        /* AF */ { &Cpu::RES_r_m0<5, Register8::A> },
        /* B0 */ { &Cpu::RES_r_m0<6, Register8::B> },
        /* B1 */ { &Cpu::RES_r_m0<6, Register8::C> },
        /* B2 */ { &Cpu::RES_r_m0<6, Register8::D> },
        /* B3 */ { &Cpu::RES_r_m0<6, Register8::E> },
        /* B4 */ { &Cpu::RES_r_m0<6, Register8::H> },
        /* B5 */ { &Cpu::RES_r_m0<6, Register8::L> },
        /* B6 */ { &Cpu::RES_arr_m0<6, Register16::HL>, &Cpu::RES_arr_m1<6, Register16::HL>, &Cpu::RES_arr_m2<6, Register16::HL> },
        /* B7 */ { &Cpu::RES_r_m0<6, Register8::A> },
        /* B8 */ { &Cpu::RES_r_m0<7, Register8::B> },
        /* B9 */ { &Cpu::RES_r_m0<7, Register8::C> },
        /* BA */ { &Cpu::RES_r_m0<7, Register8::D> },
        /* BB */ { &Cpu::RES_r_m0<7, Register8::E> },
        /* BC */ { &Cpu::RES_r_m0<7, Register8::H> },
        /* BD */ { &Cpu::RES_r_m0<7, Register8::L> },
        /* BE */ { &Cpu::RES_arr_m0<7, Register16::HL>, &Cpu::RES_arr_m1<7, Register16::HL>, &Cpu::RES_arr_m2<7, Register16::HL> },
        /* BF */ { &Cpu::RES_r_m0<7, Register8::A> },
        /* C0 */ { &Cpu::SET_r_m0<0, Register8::B> },
        /* C1 */ { &Cpu::SET_r_m0<0, Register8::C> },
        /* C2 */ { &Cpu::SET_r_m0<0, Register8::D> },
        /* C3 */ { &Cpu::SET_r_m0<0, Register8::E> },
        /* C4 */ { &Cpu::SET_r_m0<0, Register8::H> },
        /* C5 */ { &Cpu::SET_r_m0<0, Register8::L> },
        /* C6 */ { &Cpu::SET_arr_m0<0, Register16::HL>, &Cpu::SET_arr_m1<0, Register16::HL>, &Cpu::SET_arr_m2<0, Register16::HL> },
        /* C7 */ { &Cpu::SET_r_m0<0, Register8::A> },
        /* C8 */ { &Cpu::SET_r_m0<1, Register8::B> },
        /* C9 */ { &Cpu::SET_r_m0<1, Register8::C> },
        /* CA */ { &Cpu::SET_r_m0<1, Register8::D> },
        /* CB */ { &Cpu::SET_r_m0<1, Register8::E> },
        /* CC */ { &Cpu::SET_r_m0<1, Register8::H> },
        /* CD */ { &Cpu::SET_r_m0<1, Register8::L> },
        /* CE */ { &Cpu::SET_arr_m0<1, Register16::HL>, &Cpu::SET_arr_m1<1, Register16::HL>, &Cpu::SET_arr_m2<1, Register16::HL> },
        /* CF */ { &Cpu::SET_r_m0<1, Register8::A> },
        /* D0 */ { &Cpu::SET_r_m0<2, Register8::B> },
        /* D1 */ { &Cpu::SET_r_m0<2, Register8::C> },
        /* D2 */ { &Cpu::SET_r_m0<2, Register8::D> },
        /* D3 */ { &Cpu::SET_r_m0<2, Register8::E> },
        /* D4 */ { &Cpu::SET_r_m0<2, Register8::H> },
        /* D5 */ { &Cpu::SET_r_m0<2, Register8::L> },
        /* D6 */ { &Cpu::SET_arr_m0<2, Register16::HL>, &Cpu::SET_arr_m1<2, Register16::HL>, &Cpu::SET_arr_m2<2, Register16::HL> },
        /* D7 */ { &Cpu::SET_r_m0<2, Register8::A> },
        /* D8 */ { &Cpu::SET_r_m0<3, Register8::B> },
        /* D9 */ { &Cpu::SET_r_m0<3, Register8::C> },
        /* DA */ { &Cpu::SET_r_m0<3, Register8::D> },
        /* DB */ { &Cpu::SET_r_m0<3, Register8::E> },
        /* DC */ { &Cpu::SET_r_m0<3, Register8::H> },
        /* DD */ { &Cpu::SET_r_m0<3, Register8::L> },
        /* DE */ { &Cpu::SET_arr_m0<3, Register16::HL>, &Cpu::SET_arr_m1<3, Register16::HL>, &Cpu::SET_arr_m2<3, Register16::HL> },
        /* DF */ { &Cpu::SET_r_m0<3, Register8::A> },
        /* E0 */ { &Cpu::SET_r_m0<4, Register8::B> },
        /* E1 */ { &Cpu::SET_r_m0<4, Register8::C> },
        /* E2 */ { &Cpu::SET_r_m0<4, Register8::D> },
        /* E3 */ { &Cpu::SET_r_m0<4, Register8::E> },
        /* E4 */ { &Cpu::SET_r_m0<4, Register8::H> },
        /* E5 */ { &Cpu::SET_r_m0<4, Register8::L> },
        /* E6 */ { &Cpu::SET_arr_m0<4, Register16::HL>, &Cpu::SET_arr_m1<4, Register16::HL>, &Cpu::SET_arr_m2<4, Register16::HL> },
        /* E7 */ { &Cpu::SET_r_m0<4, Register8::A> },
        /* E8 */ { &Cpu::SET_r_m0<5, Register8::B> },
        /* E9 */ { &Cpu::SET_r_m0<5, Register8::C> },
        /* EA */ { &Cpu::SET_r_m0<5, Register8::D> },
        /* EB */ { &Cpu::SET_r_m0<5, Register8::E> },
        /* EC */ { &Cpu::SET_r_m0<5, Register8::H> },
        /* ED */ { &Cpu::SET_r_m0<5, Register8::L> },
        /* EE */ { &Cpu::SET_arr_m0<5, Register16::HL>, &Cpu::SET_arr_m1<5, Register16::HL>, &Cpu::SET_arr_m2<5, Register16::HL> },
        /* EF */ { &Cpu::SET_r_m0<5, Register8::A> },
        /* F0 */ { &Cpu::SET_r_m0<6, Register8::B> },
        /* F1 */ { &Cpu::SET_r_m0<6, Register8::C> },
        /* F2 */ { &Cpu::SET_r_m0<6, Register8::D> },
        /* F3 */ { &Cpu::SET_r_m0<6, Register8::E> },
        /* F4 */ { &Cpu::SET_r_m0<6, Register8::H> },
        /* F5 */ { &Cpu::SET_r_m0<6, Register8::L> },
        /* F6 */ { &Cpu::SET_arr_m0<6, Register16::HL>, &Cpu::SET_arr_m1<6, Register16::HL>, &Cpu::SET_arr_m2<6, Register16::HL> },
        /* F7 */ { &Cpu::SET_r_m0<6, Register8::A> },
        /* F8 */ { &Cpu::SET_r_m0<7, Register8::B> },
        /* F9 */ { &Cpu::SET_r_m0<7, Register8::C> },
        /* FA */ { &Cpu::SET_r_m0<7, Register8::D> },
        /* FB */ { &Cpu::SET_r_m0<7, Register8::E> },
        /* FC */ { &Cpu::SET_r_m0<7, Register8::H> },
        /* FD */ { &Cpu::SET_r_m0<7, Register8::L> },
        /* FE */ { &Cpu::SET_arr_m0<7, Register16::HL>, &Cpu::SET_arr_m1<7, Register16::HL>, &Cpu::SET_arr_m2<7, Register16::HL> },
        /* FF */ { &Cpu::SET_r_m0<7, Register8::A> },
    },
    ISR { &Cpu::ISR_m0, &Cpu::ISR_m1, &Cpu::ISR_m2, &Cpu::ISR_m3, &Cpu::ISR_m4 } // clang-format on
{
    instruction.microop.selector = instructions[0]; // start with NOP
}

void Cpu::tick() {
    // Eventually enable IME if it has been requested (by EI instruction) the cycle before
    if (IME == ImeState::Pending) {
        IME = ImeState::Enabled;
    }

    // Eventually handle pending interrupt
    if (interrupt.state == InterruptState::Pending) {
        // Decrease the remaining ticks for the pending interrupt
        if (interrupt.remainingTicks > 0) {
            --interrupt.remainingTicks;
        }

        // Handle it if the countdown is finished (but only at the beginning of a new instruction)
        if (interrupt.remainingTicks == 0 &&
            // TODO: handle CB fetch internally and use fetcher.fetching here instead
            instruction.microop.counter == 0) {
            check(halted || IME == ImeState::Enabled);

            halted = false;

            // Serve the interrupt if IME is enabled
            if (IME == ImeState::Enabled) {
                interrupt.state = InterruptState::Serving;
                serveInterrupt();
            } else {
                interrupt.state = InterruptState::None;
            }
        }
    }

    if (halted) {
        IF_DEBUGGER(++cycles);
        return;
    }

    // Eventually handle halt bug: PC will be read twice
    if (haltBug) {
        haltBug = false;
        --PC;
    }

    // Eventually fetch a new instruction
    if (fetcher.fetching) {
        fetcher.fetching = false;
        instruction.microop.selector = &fetcher.instructions[busData][0];
    }

    const MicroOperation microop = *instruction.microop.selector;

    // Advance to next micro operation
    ++instruction.microop.counter;
    ++instruction.microop.selector;

    // Execute current micro operation
    (this->*microop)();

    IF_DEBUGGER(++cycles);
}

void Cpu::checkInterrupt_t0() {
    checkInterrupt<0>();
}

void Cpu::checkInterrupt_t1() {
    checkInterrupt<1>();
}

void Cpu::checkInterrupt_t2() {
    checkInterrupt<2>();
}

void Cpu::checkInterrupt_t3() {
    checkInterrupt<3>();
}

void Cpu::saveState(Parcel& parcel) const {
    parcel.writeUInt16(AF);
    parcel.writeUInt16(BC);
    parcel.writeUInt16(DE);
    parcel.writeUInt16(HL);
    parcel.writeUInt16(PC);
    parcel.writeUInt16(SP);

    parcel.writeUInt8((uint8_t)IME);

    parcel.writeBool(halted);
    parcel.writeBool(haltBug);

    parcel.writeUInt8((uint8_t)interrupt.state);
    parcel.writeUInt8(interrupt.remainingTicks);

    parcel.writeBool(fetcher.fetching);

    if (fetcher.instructions == instructions) {
        parcel.writeUInt8(STATE_INSTRUCTION_FLAG_NORMAL);
    } else if (fetcher.instructions == instructionsCB) {
        parcel.writeUInt8(STATE_INSTRUCTION_FLAG_CB);
    } else {
        checkNoEntry();
    }

    if (static_cast<size_t>(instruction.microop.selector - &instructions[0][0]) <
        array_size(instructions) * INSTR_LEN) {
        parcel.writeUInt8(STATE_INSTRUCTION_FLAG_NORMAL);
        parcel.writeUInt16(instruction.microop.selector - &instructions[0][0]);
    } else if (static_cast<size_t>(instruction.microop.selector - &instructionsCB[0][0]) <
               array_size(instructionsCB) * INSTR_LEN) {
        parcel.writeUInt8(STATE_INSTRUCTION_FLAG_CB);
        parcel.writeUInt16(instruction.microop.selector - &instructionsCB[0][0]);
    } else if (static_cast<size_t>(instruction.microop.selector - &ISR[0]) < INSTR_LEN) {
        parcel.writeUInt8(STATE_INSTRUCTION_FLAG_ISR);
        parcel.writeUInt16(instruction.microop.selector - &ISR[0]);
    } else {
        checkNoEntry();
    }

    parcel.writeUInt8(instruction.microop.counter);
    IF_DEBUGGER(parcel.writeUInt16(instruction.address));

    parcel.writeUInt8(busData);
    parcel.writeBool(b);
    parcel.writeUInt8(u);
    parcel.writeUInt8(u2);
    parcel.writeUInt8(lsb);
    parcel.writeUInt8(msb);
    parcel.writeUInt16(uu);
    parcel.writeUInt16(addr);

    IF_DEBUGGER(parcel.writeUInt64(cycles));
}

void Cpu::loadState(Parcel& parcel) {
    AF = parcel.readInt16();
    BC = parcel.readInt16();
    DE = parcel.readInt16();
    HL = parcel.readInt16();
    PC = parcel.readInt16();
    SP = parcel.readInt16();

    IME = (ImeState)(parcel.readUInt8());

    halted = parcel.readBool();
    haltBug = parcel.readBool();

    interrupt.state = (InterruptState)parcel.readUInt8();
    interrupt.remainingTicks = parcel.readUInt8();

    fetcher.fetching = parcel.readBool();

    const uint8_t instructionsFlag = parcel.readUInt8();
    if (instructionsFlag == STATE_INSTRUCTION_FLAG_NORMAL) {
        fetcher.instructions = instructions;
    } else if (instructionsFlag == STATE_INSTRUCTION_FLAG_CB) {
        fetcher.instructions = instructionsCB;
    } else {
        checkNoEntry();
    }

    const uint8_t instructionFlag = parcel.readUInt8();
    const uint16_t offset = parcel.readInt16();

    if (instructionFlag == STATE_INSTRUCTION_FLAG_NORMAL)
        instruction.microop.selector = &instructions[offset / INSTR_LEN][offset % INSTR_LEN];
    else if (instructionFlag == STATE_INSTRUCTION_FLAG_CB)
        instruction.microop.selector = &instructionsCB[offset / INSTR_LEN][offset % INSTR_LEN];
    else if (instructionFlag == STATE_INSTRUCTION_FLAG_ISR)
        instruction.microop.selector = &ISR[offset];
    else
        checkNoEntry();

    instruction.microop.counter = parcel.readUInt8();
    IF_DEBUGGER(instruction.address = parcel.readUInt16());

    busData = parcel.readUInt8();
    b = parcel.readBool();
    u = parcel.readUInt8();
    u2 = parcel.readUInt8();
    lsb = parcel.readUInt8();
    msb = parcel.readUInt8();
    uu = parcel.readUInt16();
    addr = parcel.readUInt16();

    IF_DEBUGGER(cycles = parcel.readUInt64());
}

template <uint8_t t>
void Cpu::checkInterrupt() {
    /*
     * Interrupt timings (blank spaces are unknown).
     *
     * -----------------------------------
     * NotHalted | T0  |  T1 |  T2 |  T3 |
     * -----------------------------------
     * VBlank    |  1  | 1/2 |     |     |
     * Stat      |  1  |  1  |  1  |  2  |
     * Timer     |  1  | 1/2 |     |  2  |
     * Serial    |  1  | 1/2 |     |  2  |
     * Joypad    | 1/2 | 1/2 |     |     |
     *
     * -----------------------------------
     * Halted    | T0  |  T1 |  T2 |  T3 |
     * -----------------------------------
     * VBlank    |  1  |     |     |     |
     * Stat      |  1  |  2  |  2  |  2  |
     * Timer     |     |     |     |  3  |
     * Serial    |     |     |     |  3  |
     * Joypad    |     |     |     |     |
     */

    IF_ASSERTS(static constexpr uint8_t UNKNOWN_INTERRUPT_TIMING = UINT8_MAX);
    static constexpr uint8_t U = IF_ASSERTS_ELSE(UNKNOWN_INTERRUPT_TIMING, 1);
    static constexpr uint8_t J = 1; // not sure about joypad timing

    // clang-format off
    static constexpr uint8_t INTERRUPTS_TIMINGS[32][2][4] /* [interrupt flags][halted][t-cycle] */ = {
        /*  0 : None */ {{U, U, U, U}, {U, U, U, U}},
        /*  1 : VBlank */ {{1, 1, U, U}, {1, U, U, U}},
        /*  2 : STAT */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /*  3 : STAT + VBlank */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /*  4 : Timer */ {{1, 1, U, 2}, {U, U, U, 3}},
        /*  5 : Timer + VBlank */ {{1, 1, U, 2}, {1, U, U, 3}},
        /*  6 : Timer + STAT */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /*  7 : Timer + STAT + VBlank */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /*  8 : Serial */ {{1, 1, U, 2}, {U, U, U, 3}},
        /*  9 : Serial + VBlank */ {{1, 1, U, 2}, {1, U, U, 3}},
        /* 10 : Serial + STAT */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /* 11 : Serial + STAT + VBlank */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /* 12 : Serial + Timer */ {{1, 1, U, 2}, {U, U, U, 3}},
        /* 13 : Serial + Timer + VBlank */ {{1, 1, U, 2}, {1, U, U, 3}},
        /* 14 : Serial + Timer + STAT */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /* 15 : Serial + Timer + STAT + VBlank */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /* 16 : Joypad */ {{J, J, J, J}, {J, J, J, J}},
        /* 17 : Joypad + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 18 : Joypad + STAT */ {{J, J, J, J}, {J, J, J, J}},
        /* 19 : Joypad + STAT + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 20 : Joypad + Timer */ {{J, J, J, J}, {J, J, J, J}},
        /* 21 : Joypad + Timer + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 22 : Joypad + Timer + STAT */ {{J, J, J, J}, {J, J, J, J}},
        /* 23 : Joypad + Timer + STAT + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 24 : Joypad + Serial */ {{J, J, J, J}, {J, J, J, J}},
        /* 25 : Joypad + Serial + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 26 : Joypad + Serial + STAT */ {{J, J, J, J}, {J, J, J, J}},
        /* 27 : Joypad + Serial + STAT + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 28 : Joypad + Serial + Timer */ {{J, J, J, J}, {J, J, J, J}},
        /* 29 : Joypad + Serial + Timer + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 30 : Joypad + Serial + Timer + STAT */ {{J, J, J, J}, {J, J, J, J}},
        /* 31 : Joypad + Serial + Timer + STAT + VBlank */ {{J, J, J, J}, {J, J, J, J}},
    };
    // clang-format on

    if (interrupt.state == InterruptState::None && (halted || IME == ImeState::Enabled)) {
        if (const uint8_t pendingInterrupts = getPendingInterrupts()) {
            interrupt.state = InterruptState::Pending;
            interrupt.remainingTicks = INTERRUPTS_TIMINGS[pendingInterrupts][halted][t];
            checkCode({
                if (interrupt.remainingTicks == UNKNOWN_INTERRUPT_TIMING) {
                    std::cerr << "---- UNKNOWN INTERRUPT TIMING ----" << std::endl;
                    std::cerr << "PC     : " << hex(PC) << std::endl;
                    std::cerr << "Cycle  : " << cycles << std::endl;
                    std::cerr << "Halted : " << halted << std::endl;
                    std::cerr << "T      : " << +t << std::endl;
                    std::cerr << "IF     : " << bin((uint8_t)interrupts.IF) << std::endl;
                    std::cerr << "IE     : " << bin((uint8_t)interrupts.IE) << std::endl;
                    std::cerr << "VBlank : " << test_bit<Specs::Bits::Interrupts::VBLANK>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    std::cerr << "STAT   : " << test_bit<Specs::Bits::Interrupts::STAT>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    std::cerr << "TIMER  : " << test_bit<Specs::Bits::Interrupts::TIMER>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    std::cerr << "SERIAL : " << test_bit<Specs::Bits::Interrupts::SERIAL>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    std::cerr << "JOYPAD : " << test_bit<Specs::Bits::Interrupts::JOYPAD>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    fatal("Unknown interrupt timing: this should be fixed by the developer");
                }
            });
        }
    }
}

inline uint8_t Cpu::getPendingInterrupts() const {
    return interrupts.IE & interrupts.IF & bitmask<5>;
}

void Cpu::serveInterrupt() {
    check(fetcher.fetching);
    check(interrupt.state == InterruptState::Serving);
    fetcher.fetching = false;
    instruction.microop.counter = 0;
    instruction.microop.selector = &ISR[0];
}

inline void Cpu::fetch() {
    IF_DEBUGGER(instruction.address = PC);
    instruction.microop.counter = 0;
    fetcher.fetching = true;
    fetcher.instructions = instructions;
    mmu.requestRead(PC++);
}

inline void Cpu::fetchCB() {
    IF_DEBUGGER(instruction.address = PC);
    check(instruction.microop.counter == 1);
    fetcher.fetching = true;
    fetcher.instructions = instructionsCB;
    mmu.requestRead(PC++);
}

template <Cpu::Register8 r>
uint8_t Cpu::readRegister8() const {
    if constexpr (r == Register8::A) {
        return get_byte<1>(AF);
    } else if constexpr (r == Register8::B) {
        return get_byte<1>(BC);
    } else if constexpr (r == Register8::C) {
        return get_byte<0>(BC);
    } else if constexpr (r == Register8::D) {
        return get_byte<1>(DE);
    } else if constexpr (r == Register8::E) {
        return get_byte<0>(DE);
    } else if constexpr (r == Register8::F) {
        return get_byte<0>(AF) & 0xF0; // last four bits hardwired to 0
    } else if constexpr (r == Register8::H) {
        return get_byte<1>(HL);
    } else if constexpr (r == Register8::L) {
        return get_byte<0>(HL);
    } else if constexpr (r == Register8::SP_S) {
        return get_byte<1>(SP);
    } else if constexpr (r == Register8::SP_P) {
        return get_byte<0>(SP);
    } else if constexpr (r == Register8::PC_P) {
        return get_byte<1>(PC);
    } else if constexpr (r == Register8::PC_C) {
        return get_byte<0>(PC);
    }
}

template <Cpu::Register8 r>
void Cpu::writeRegister8(uint8_t value) {
    if constexpr (r == Register8::A) {
        set_byte<1>(AF, value);
    } else if constexpr (r == Register8::B) {
        set_byte<1>(BC, value);
    } else if constexpr (r == Register8::C) {
        set_byte<0>(BC, value);
    } else if constexpr (r == Register8::D) {
        set_byte<1>(DE, value);
    } else if constexpr (r == Register8::E) {
        set_byte<0>(DE, value);
    } else if constexpr (r == Register8::F) {
        set_byte<0>(AF, value & 0xF0);
    } else if constexpr (r == Register8::H) {
        set_byte<1>(HL, value);
    } else if constexpr (r == Register8::L) {
        set_byte<0>(HL, value);
    } else if constexpr (r == Register8::SP_S) {
        set_byte<1>(SP, value);
    } else if constexpr (r == Register8::SP_P) {
        set_byte<0>(SP, value);
    } else if constexpr (r == Register8::PC_P) {
        set_byte<1>(PC, value);
    } else if constexpr (r == Register8::PC_C) {
        set_byte<0>(PC, value);
    }
}

template <Cpu::Register16 rr>
uint16_t Cpu::readRegister16() const {
    if constexpr (rr == Register16::AF) {
        return AF & 0xFFF0;
    } else if constexpr (rr == Register16::BC) {
        return BC;
    } else if constexpr (rr == Register16::DE) {
        return DE;
    } else if constexpr (rr == Register16::HL) {
        return HL;
    } else if constexpr (rr == Register16::PC) {
        return PC;
    } else if constexpr (rr == Register16::SP) {
        return SP;
    }
}

template <Cpu::Register16 rr>
void Cpu::writeRegister16(uint16_t value) {
    if constexpr (rr == Register16::AF) {
        AF = value & 0xFFF0;
    } else if constexpr (rr == Register16::BC) {
        BC = value;
    } else if constexpr (rr == Register16::DE) {
        DE = value;
    } else if constexpr (rr == Register16::HL) {
        HL = value;
    } else if constexpr (rr == Register16::PC) {
        PC = value;
    } else if constexpr (rr == Register16::SP) {
        SP = value;
    }
}

template <Cpu::Flag f>
[[nodiscard]] bool Cpu::testFlag() const {
    if constexpr (f == Flag::Z) {
        return get_bit<Specs::Bits::Flags::Z>(AF);
    } else if constexpr (f == Flag::N) {
        return get_bit<Specs::Bits::Flags::N>(AF);
    } else if constexpr (f == Flag::H) {
        return get_bit<Specs::Bits::Flags::H>(AF);
    } else if constexpr (f == Flag::C) {
        return get_bit<Specs::Bits::Flags::C>(AF);
    }
}

template <Cpu::Flag f, bool y>
[[nodiscard]] bool Cpu::checkFlag() const {
    return testFlag<f>() == y;
}

template <Cpu::Flag f>
void Cpu::setFlag() {
    if constexpr (f == Flag::Z) {
        set_bit<Specs::Bits::Flags::Z>(AF);
    } else if constexpr (f == Flag::N) {
        set_bit<Specs::Bits::Flags::N>(AF);
    } else if constexpr (f == Flag::H) {
        set_bit<Specs::Bits::Flags::H>(AF);
    } else if constexpr (f == Flag::C) {
        set_bit<Specs::Bits::Flags::C>(AF);
    }
}

template <Cpu::Flag f>
void Cpu::setFlag(bool value) {
    if constexpr (f == Flag::Z) {
        set_bit<Specs::Bits::Flags::Z>(AF, value);
    } else if constexpr (f == Flag::N) {
        set_bit<Specs::Bits::Flags::N>(AF, value);
    } else if constexpr (f == Flag::H) {
        set_bit<Specs::Bits::Flags::H>(AF, value);
    } else if constexpr (f == Flag::C) {
        set_bit<Specs::Bits::Flags::C>(AF, value);
    }
}

template <Cpu::Flag f>
void Cpu::resetFlag() {
    if constexpr (f == Flag::Z) {
        reset_bit<Specs::Bits::Flags::Z>(AF);
    } else if constexpr (f == Flag::N) {
        reset_bit<Specs::Bits::Flags::N>(AF);
    } else if constexpr (f == Flag::H) {
        reset_bit<Specs::Bits::Flags::H>(AF);
    } else if constexpr (f == Flag::C) {
        reset_bit<Specs::Bits::Flags::C>(AF);
    }
}

// ============================= INSTRUCTIONS ==================================

void Cpu::invalidMicroOperation() { // NOLINT(readability-make-member-function-const)
    fatal("Invalid micro operation at address " + hex(PC));
}

void Cpu::ISR_m0() {
    check(interrupt.state == InterruptState::Serving);

    uu = PC - 1 /* -1 because of prefetch */;
    mmu.requestWrite(--SP, get_byte<1>(uu));
}

void Cpu::ISR_m1() {
    // The reads of IE must happen after the high byte of PC has been pushed
    u = interrupts.IE;
}

void Cpu::ISR_m2() {
    // The reads of IF must happen after the high byte of PC has been pushed
    u2 = interrupts.IF;
}

void Cpu::ISR_m3() {
    const uint8_t IE_and_IF = u & u2;

    // There is an opportunity to cancel the interrupt and jump to address 0x0000
    // if the flag in IF/IE has been reset between the trigger of the interrupt
    // and this moment (reference not found, copied from other emu)
    uint16_t nextPC = 0x0000;

    for (uint8_t b = 0; b <= 4; b++) {
        if (test_bit(IE_and_IF, b)) {
            reset_bit(interrupts.IF, b);
            nextPC = 0x40 + 8 * b;
            break;
        }
    }

    PC = nextPC;

    // The push of the low byte of PC must happen after the IE/IF flags have been read
    mmu.requestWrite(--SP, get_byte<0>(uu));
}

void Cpu::ISR_m4() {
    IME = ImeState::Disabled;
    interrupt.state = InterruptState::None;
    fetch();
}

void Cpu::NOP_m0() {
    fetch();
}

void Cpu::STOP_m0() {
    // TODO: what to do here?
    fetch();
}

void Cpu::HALT_m0() {
    check(!halted);
    check(interrupt.state != InterruptState::Serving);

    if (getPendingInterrupts()) {
        haltBug = IME == ImeState::Enabled;
    } else {
        halted = true;
    }

    fetch();
}

void Cpu::DI_m0() {
    IME = ImeState::Disabled;
    fetch();
}

void Cpu::EI_m0() {
    if (IME == ImeState::Disabled) {
        IME = ImeState::Pending;
    }
    fetch();
}

// e.g. 01 | LD BC,d16

template <Cpu::Register16 rr>
void Cpu::LD_rr_uu_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register16 rr>
void Cpu::LD_rr_uu_m1() {
    lsb = busData;
    mmu.requestRead(PC++);
}

template <Cpu::Register16 rr>
void Cpu::LD_rr_uu_m2() {
    writeRegister16<rr>(concat(busData, lsb));
    fetch();
}

// e.g. 36 | LD (HL),d8

template <Cpu::Register16 rr>
void Cpu::LD_arr_u_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register16 rr>
void Cpu::LD_arr_u_m1() {
    mmu.requestWrite(readRegister16<rr>(), busData);
}

template <Cpu::Register16 rr>
void Cpu::LD_arr_u_m2() {
    fetch();
}

// e.g. 02 | LD (BC),A

template <Cpu::Register16 rr, Cpu::Register8 r>
void Cpu::LD_arr_r_m0() {
    mmu.requestWrite(readRegister16<rr>(), readRegister8<r>());
}

template <Cpu::Register16 rr, Cpu::Register8 r>
void Cpu::LD_arr_r_m1() {
    fetch();
}

// e.g. 22 | LD (HL+),A

template <Cpu::Register16 rr, Cpu::Register8 r, int8_t inc>
void Cpu::LD_arri_r_m0() {
    mmu.requestWrite(readRegister16<rr>(), readRegister8<r>());
    writeRegister16<rr>(readRegister16<rr>() + inc);
}

template <Cpu::Register16 rr, Cpu::Register8 r, int8_t inc>
void Cpu::LD_arri_r_m1() {
    fetch();
}

// e.g. 06 | LD B,d8

template <Cpu::Register8 r>
void Cpu::LD_r_u_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register8 r>
void Cpu::LD_r_u_m1() {
    writeRegister8<r>(busData);
    fetch();
}

// e.g. 08 | LD (a16),SP

template <Cpu::Register16 rr>
void Cpu::LD_ann_rr_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register16 rr>
void Cpu::LD_ann_rr_m1() {
    lsb = busData;
    mmu.requestRead(PC++);
}

template <Cpu::Register16 rr>
void Cpu::LD_ann_rr_m2() {
    addr = concat(busData, lsb);
    uu = readRegister16<rr>();
    mmu.requestWrite(addr, get_byte<0>(uu));
}

template <Cpu::Register16 rr>
void Cpu::LD_ann_rr_m3() {
    mmu.requestWrite(addr + 1, get_byte<1>(uu));
}

template <Cpu::Register16 rr>
void Cpu::LD_ann_rr_m4() {
    fetch();
}

// e.g. 0A |  LD A,(BC)

template <Cpu::Register8 r, Cpu::Register16 rr>
void Cpu::LD_r_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register8 r, Cpu::Register16 rr>
void Cpu::LD_r_arr_m1() {
    writeRegister8<r>(busData);
    fetch();
}

// e.g. 2A |  LD A,(HL+)

template <Cpu::Register8 r, Cpu::Register16 rr, int8_t inc>
void Cpu::LD_r_arri_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register8 r, Cpu::Register16 rr, int8_t inc>
void Cpu::LD_r_arri_m1() {
    writeRegister8<r>(busData);
    writeRegister16<rr>(readRegister16<rr>() + inc);
    fetch();
}

// e.g. 41 |  LD B,C

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LD_r_r_m0() {
    writeRegister8<r1>(readRegister8<r2>());
    fetch();
}

// e.g. E0 | LDH (a8),A

template <Cpu::Register8 r>
void Cpu::LDH_an_r_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register8 r>
void Cpu::LDH_an_r_m1() {
    mmu.requestWrite(concat(0xFF, busData), readRegister8<r>());
}

template <Cpu::Register8 r>
void Cpu::LDH_an_r_m2() {
    fetch();
}

// e.g. F0 | LDH A,(a8)

template <Cpu::Register8 r>
void Cpu::LDH_r_an_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register8 r>
void Cpu::LDH_r_an_m1() {
    mmu.requestRead(concat(0xFF, busData));
}

template <Cpu::Register8 r>
void Cpu::LDH_r_an_m2() {
    writeRegister8<r>(busData);
    fetch();
}

// e.g. E2 | LD (C),A

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LDH_ar_r_m0() {
    mmu.requestWrite(concat(0xFF, readRegister8<r1>()), readRegister8<r2>());
}

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LDH_ar_r_m1() {
    fetch();
}

// e.g. F2 | LD A,(C)

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LDH_r_ar_m0() {
    mmu.requestRead(concat(0xFF, readRegister8<r2>()));
}

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LDH_r_ar_m1() {
    writeRegister8<r1>(busData);
    fetch();
}

// e.g. EA | LD (a16),A

template <Cpu::Register8 r>
void Cpu::LD_ann_r_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register8 r>
void Cpu::LD_ann_r_m1() {
    lsb = busData;
    mmu.requestRead(PC++);
}

template <Cpu::Register8 r>
void Cpu::LD_ann_r_m2() {
    mmu.requestWrite(concat(busData, lsb), readRegister8<r>());
}

template <Cpu::Register8 r>
void Cpu::LD_ann_r_m3() {
    fetch();
}

// e.g. FA | LD A,(a16)

template <Cpu::Register8 r>
void Cpu::LD_r_ann_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register8 r>
void Cpu::LD_r_ann_m1() {
    lsb = busData;
    mmu.requestRead(PC++);
}

template <Cpu::Register8 r>
void Cpu::LD_r_ann_m2() {
    mmu.requestRead(concat(busData, lsb));
}

template <Cpu::Register8 r>
void Cpu::LD_r_ann_m3() {
    writeRegister8<r>(busData);
    fetch();
}

// e.g. F9 | LD SP,HL

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rr_m0() {
    writeRegister16<rr1>(readRegister16<rr2>());
    // TODO: i guess this should be split in the two 8bit registers between m1 and m2
}

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rr_m1() {
    fetch();
}

// e.g. F8 | LD HL,SP+r8

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rrs_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rrs_m1() {
    uu = readRegister16<rr2>();
    auto [result, h, c] = sum_carry<3, 7>(uu, to_signed(busData));
    writeRegister16<rr1>(result);
    resetFlag<(Flag::Z)>();
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    // TODO: when are flags set?
    // TODO: does it work?
}

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rrs_m2() {
    fetch();
}

// e.g. 04 | INC B

template <Cpu::Register8 r>
void Cpu::INC_r_m0() {
    uint8_t tmp = readRegister8<r>();
    auto [result, h] = sum_carry<3>(tmp, 1);
    writeRegister8<r>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<Flag::N>();
    setFlag<Flag::H>(h);
    fetch();
}

// e.g. 03 | INC BC

template <Cpu::Register16 rr>
void Cpu::INC_rr_m0() {
    uint16_t tmp = readRegister16<rr>();
    uint32_t result = tmp + 1;
    writeRegister16<rr>(result);
    // TODO: no flags?
}

template <Cpu::Register16 rr>
void Cpu::INC_rr_m1() {
    fetch();
}

// e.g. 34 | INC (HL)

template <Cpu::Register16 rr>
void Cpu::INC_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::INC_arr_m1() {
    auto [result, h] = sum_carry<3>(busData, 1);
    mmu.requestWrite(addr, result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
}

template <Cpu::Register16 rr>
void Cpu::INC_arr_m2() {
    fetch();
}

// e.g. 05 | DEC B

template <Cpu::Register8 r>
void Cpu::DEC_r_m0() {
    uint8_t tmp = readRegister8<r>();
    auto [result, h] = sub_borrow<3>(tmp, 1);
    writeRegister8<r>(result);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    fetch();
}

// e.g. 0B | DEC BC

template <Cpu::Register16 rr>
void Cpu::DEC_rr_m0() {
    uint16_t tmp = readRegister16<rr>();
    uint32_t result = tmp - 1;
    writeRegister16<rr>(result);
    // TODO: no flags?
}

template <Cpu::Register16 rr>
void Cpu::DEC_rr_m1() {
    fetch();
}

// e.g. 35 | DEC (HL)

template <Cpu::Register16 rr>
void Cpu::DEC_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::DEC_arr_m1() {
    auto [result, h] = sub_borrow<3>(busData, 1);
    mmu.requestWrite(addr, result);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
}

template <Cpu::Register16 rr>
void Cpu::DEC_arr_m2() {
    fetch();
}

// e.g. 80 | ADD B

template <Cpu::Register8 r>
void Cpu::ADD_r_m0() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

// e.g. 86 | ADD (HL)

template <Cpu::Register16 rr>
void Cpu::ADD_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::ADD_arr_m1() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), busData);
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

// C6 | ADD A,d8

void Cpu::ADD_u_m0() {
    mmu.requestRead(PC++);
}

void Cpu::ADD_u_m1() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), busData);
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

// e.g. 88 | ADC B

template <Cpu::Register8 r>
void Cpu::ADC_r_m0() {
    // TODO: dont like this + C very much...
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    auto [result, h, c] = sum_carry<3, 7>(tmp, testFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h | h0);
    setFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 8E | ADC (HL)

template <Cpu::Register16 rr>
void Cpu::ADC_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::ADC_arr_m1() {
    // TODO: is this ok?
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), busData);
    auto [result, h, c] = sum_carry<3, 7>(tmp, testFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h | h0);
    setFlag<Flag::C>(c | c0);
    fetch();
}

// CE | ADC A,d8

void Cpu::ADC_u_m0() {
    mmu.requestRead(PC++);
}

void Cpu::ADC_u_m1() {
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), busData);
    auto [result, h, c] = sum_carry<3, 7>(tmp, testFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h | h0);
    setFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 09 | ADD HL,BC

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ADD_rr_rr_m0() {
    auto [result, h, c] = sum_carry<11, 15>(readRegister16<rr1>(), readRegister16<rr2>());
    writeRegister16<rr1>(result);
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
}

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ADD_rr_rr_m1() {
    fetch();
}

// e.g. E8 | ADD SP,r8

template <Cpu::Register16 rr>
void Cpu::ADD_rr_s_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Register16 rr>
void Cpu::ADD_rr_s_m1() {
    // TODO: is it ok to carry bit 3 and 7?
    auto [result, h, c] = sum_carry<3, 7>(readRegister16<rr>(), to_signed(busData));
    writeRegister16<rr>(result);
    resetFlag<(Flag::Z)>();
    resetFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
}

template <Cpu::Register16 rr>
void Cpu::ADD_rr_s_m2() {
    // TODO: why? i guess something about the instruction timing is wrong
}

template <Cpu::Register16 rr>
void Cpu::ADD_rr_s_m3() {
    fetch();
}

// e.g. 90 | SUB B

template <Cpu::Register8 r>
void Cpu::SUB_r_m0() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

// e.g. 96 | SUB (HL)

template <Cpu::Register16 rr>
void Cpu::SUB_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::SUB_arr_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), busData);
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

// D6 | SUB A,d8

void Cpu::SUB_u_m0() {
    mmu.requestRead(PC++);
}

void Cpu::SUB_u_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), busData);
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

// e.g. 98 | SBC B

template <Cpu::Register8 r>
void Cpu::SBC_r_m0() {
    // TODO: is this ok?
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +testFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h | h0);
    setFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 9E | SBC A,(HL)

template <Cpu::Register16 rr>
void Cpu::SBC_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::SBC_arr_m1() {
    // TODO: dont like this - C very much...
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), busData);
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +testFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h | h0);
    setFlag<Flag::C>(c | c0);
    fetch();
}

// D6 | SBC A,d8

void Cpu::SBC_u_m0() {
    mmu.requestRead(PC++);
}

void Cpu::SBC_u_m1() {
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), busData);
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +testFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h | h0);
    setFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. A0 | AND B

template <Cpu::Register8 r>
void Cpu::AND_r_m0() {
    uint8_t result = readRegister8<Register8::A>() & readRegister8<r>();
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

// e.g. A6 | AND (HL)

template <Cpu::Register16 rr>
void Cpu::AND_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::AND_arr_m1() {
    uint8_t result = readRegister8<Register8::A>() & busData;
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

// E6 | AND d8

void Cpu::AND_u_m0() {
    mmu.requestRead(PC++);
}

void Cpu::AND_u_m1() {
    uint8_t result = readRegister8<Register8::A>() & busData;
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    setFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

// B0 | OR B

template <Cpu::Register8 r>
void Cpu::OR_r_m0() {
    uint8_t result = readRegister8<Register8::A>() | readRegister8<r>();
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

// e.g. B6 | OR (HL)

template <Cpu::Register16 rr>
void Cpu::OR_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::OR_arr_m1() {
    uint8_t result = readRegister8<Register8::A>() | busData;
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

// F6 | OR d8

void Cpu::OR_u_m0() {
    mmu.requestRead(PC++);
}

void Cpu::OR_u_m1() {
    uint8_t result = readRegister8<Register8::A>() | busData;
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

// e.g. A8 | XOR B

template <Cpu::Register8 r>
void Cpu::XOR_r_m0() {
    uint8_t result = readRegister8<Register8::A>() ^ readRegister8<r>();
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

template <Cpu::Register16 rr>
void Cpu::XOR_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::XOR_arr_m1() {
    uint8_t result = readRegister8<Register8::A>() ^ busData;
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

// EE | XOR d8

void Cpu::XOR_u_m0() {
    mmu.requestRead(PC++);
}

void Cpu::XOR_u_m1() {
    uint8_t result = readRegister8<Register8::A>() ^ busData;
    writeRegister8<Register8::A>(result);
    setFlag<Flag::Z>(result == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

template <Cpu::Register8 r>
void Cpu::CP_r_m0() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

template <Cpu::Register16 rr>
void Cpu::CP_arr_m0() {
    mmu.requestRead(readRegister16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::CP_arr_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), busData);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

// FE | CP d8

void Cpu::CP_u_m0() {
    mmu.requestRead(PC++);
}

void Cpu::CP_u_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), busData);
    setFlag<Flag::Z>(result == 0);
    setFlag<(Flag::N)>();
    setFlag<Flag::H>(h);
    setFlag<Flag::C>(c);
    fetch();
}

// 27 | DAA

void Cpu::DAA_m0() {
    u = readRegister8<Register8::A>();
    auto N = testFlag<Flag::N>();
    auto H = testFlag<Flag::H>();
    auto C = testFlag<Flag::C>();

    if (!N) {
        if (C || u > 0x99) {
            u += 0x60;
            C = true;
        }
        if (H || get_nibble<0>(u) > 0x9)
            u += 0x06;
    } else {
        if (C)
            u -= 0x60;
        if (H)
            u -= 0x06;
    }
    writeRegister8<Register8::A>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(C);
    fetch();
}

// 37 | SCF

void Cpu::SCF_m0() {
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<(Flag::C)>();
    fetch();
}

// 2F | CPL

void Cpu::CPL_m0() {
    writeRegister8<Register8::A>(~readRegister8<Register8::A>());
    setFlag<(Flag::N)>();
    setFlag<(Flag::H)>();
    fetch();
}

// 3F | CCF_m0

void Cpu::CCF_m0() {
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(!testFlag<Flag::C>());
    fetch();
}

// 07 | RLCA

void Cpu::RLCA_m0() {
    u = readRegister8<Register8::A>();
    bool b7 = test_bit<7>(u);
    u = (u << 1) | b7;
    writeRegister8<Register8::A>(u);
    resetFlag<(Flag::Z)>();
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b7);
    fetch();
}

// 17 | RLA

void Cpu::RLA_m0() {
    u = readRegister8<Register8::A>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | testFlag<Flag::C>();
    writeRegister8<Register8::A>(u);
    resetFlag<(Flag::Z)>();
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b7);
    fetch();
}

// 0F | RRCA

void Cpu::RRCA_m0() {
    u = readRegister8<Register8::A>();
    bool b0 = test_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    writeRegister8<Register8::A>(u);
    resetFlag<(Flag::Z)>();
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
    fetch();
}

// 1F | RRA

void Cpu::RRA_m0() {
    u = readRegister8<Register8::A>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (testFlag<Flag::C>() << 7);
    writeRegister8<Register8::A>(u);
    resetFlag<(Flag::Z)>();
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
    fetch();
}

// e.g. 18 | JR r8

void Cpu::JR_s_m0() {
    mmu.requestRead(PC++);
}

void Cpu::JR_s_m1() {
    PC = (int16_t)PC + to_signed(busData);
}

void Cpu::JR_s_m2() {
    fetch();
}

// e.g. 28 | JR Z,r8
// e.g. 20 | JR NZ,r8

template <Cpu::Flag f, bool y>
void Cpu::JR_c_s_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Flag f, bool y>
void Cpu::JR_c_s_m1() {
    if (checkFlag<f, y>()) {
        PC = PC + to_signed(busData);
    } else {
        fetch();
    }
}

template <Cpu::Flag f, bool y>
void Cpu::JR_c_s_m2() {
    fetch();
}

// e.g. C3 | JP a16

void Cpu::JP_uu_m0() {
    mmu.requestRead(PC++);
}

void Cpu::JP_uu_m1() {
    lsb = busData;
    mmu.requestRead(PC++);
}

void Cpu::JP_uu_m2() {
    PC = concat(busData, lsb);
}

void Cpu::JP_uu_m3() {
    fetch();
}

// e.g. E9 | JP (HL)

template <Cpu::Register16 rr>
void Cpu::JP_rr_m0() {
    PC = readRegister16<rr>();
    fetch();
}

// e.g. CA | JP Z,a16
// e.g. C2 | JP NZ,a16

template <Cpu::Flag f, bool y>
void Cpu::JP_c_uu_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Flag f, bool y>
void Cpu::JP_c_uu_m1() {
    lsb = busData;
    mmu.requestRead(PC++);
}

template <Cpu::Flag f, bool y>
void Cpu::JP_c_uu_m2() {
    if (checkFlag<f, y>()) {
        PC = concat(busData, lsb);
    } else {
        fetch();
    }
}

template <Cpu::Flag f, bool y>
void Cpu::JP_c_uu_m3() {
    fetch();
}

// CD | CALL a16

void Cpu::CALL_uu_m0() {
    mmu.requestRead(PC++);
}

void Cpu::CALL_uu_m1() {
    lsb = busData;
    mmu.requestRead(PC++);
}

void Cpu::CALL_uu_m2() {
    msb = busData;
    // internal delay
}

void Cpu::CALL_uu_m3() {
    mmu.requestWrite(--SP, readRegister8<Register8::PC_P>());
}

void Cpu::CALL_uu_m4() {
    mmu.requestWrite(--SP, readRegister8<Register8::PC_C>());
}

void Cpu::CALL_uu_m5() {
    PC = concat(msb, lsb);
    fetch();
}

// e.g. C4 | CALL NZ,a16

template <Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m0() {
    mmu.requestRead(PC++);
}

template <Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m1() {
    lsb = busData;
    mmu.requestRead(PC++);
}
template <Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m2() {
    if (!checkFlag<f, y>()) {
        fetch();
    } else {
        // else: internal delay
        msb = busData;
    }
}

template <Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m3() {
    // TODO: here or next cycle?
    mmu.requestWrite(--SP, readRegister8<Register8::PC_P>());
}

template <Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m4() {
    mmu.requestWrite(--SP, readRegister8<Register8::PC_C>());
}

template <Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m5() {
    PC = concat(msb, lsb);
    fetch();
}

// C7 | RST 00H

template <uint8_t n>
void Cpu::RST_m0() {
    // internal delay
}

template <uint8_t n>
void Cpu::RST_m1() {
    mmu.requestWrite(--SP, readRegister8<Register8::PC_P>());
}

template <uint8_t n>
void Cpu::RST_m2() {
    mmu.requestWrite(--SP, readRegister8<Register8::PC_C>());
}

template <uint8_t n>
void Cpu::RST_m3() {
    PC = concat(0x00, n);
    fetch();
}

// C9 | RET

void Cpu::RET_uu_m0() {
    mmu.requestRead(SP++);
}

void Cpu::RET_uu_m1() {
    lsb = busData;
    mmu.requestRead(SP++);
}

void Cpu::RET_uu_m2() {
    PC = concat(busData, lsb);
}

void Cpu::RET_uu_m3() {
    fetch();
}

// D9 | RETI

void Cpu::RETI_uu_m0() {
    mmu.requestRead(SP++);
}

void Cpu::RETI_uu_m1() {
    lsb = busData;
    mmu.requestRead(SP++);
}

void Cpu::RETI_uu_m2() {
    PC = concat(busData, lsb);
    IME = ImeState::Enabled;
}

void Cpu::RETI_uu_m3() {
    fetch();
}

// e.g. C0 | RET NZ

template <Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m0() {
}

template <Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m1() {
    // TODO: really bad but don't know why this lasts 2 m cycle if false
    if (checkFlag<f, y>()) {
        mmu.requestRead(SP++);
    } else {
        fetch();
    }
}

template <Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m2() {
    lsb = busData;
    mmu.requestRead(SP++);
}

template <Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m3() {
    PC = concat(busData, lsb);
}

template <Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m4() {
    fetch();
}

// e.g. C5 | PUSH BC

template <Cpu::Register16 rr>
void Cpu::PUSH_rr_m0() {
    uu = readRegister16<rr>();
}

template <Cpu::Register16 rr>
void Cpu::PUSH_rr_m1() {
    mmu.requestWrite(--SP, get_byte<1>(uu));
}

template <Cpu::Register16 rr>
void Cpu::PUSH_rr_m2() {
    mmu.requestWrite(--SP, get_byte<0>(uu));
}

template <Cpu::Register16 rr>
void Cpu::PUSH_rr_m3() {
    fetch();
}

// e.g. C1 | POP BC

template <Cpu::Register16 rr>
void Cpu::POP_rr_m0() {
    mmu.requestRead(SP++);
}

template <Cpu::Register16 rr>
void Cpu::POP_rr_m1() {
    lsb = busData;
    mmu.requestRead(SP++);
}

template <Cpu::Register16 rr>
void Cpu::POP_rr_m2() {
    writeRegister16<rr>(concat(busData, lsb));
    fetch();
}

void Cpu::CB_m0() {
    fetchCB();
}

// e.g. CB 00 | RLC B

template <Cpu::Register8 r>
void Cpu::RLC_r_m0() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | b7;
    writeRegister8<r>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b7);
    fetch();
}

// e.g. CB 06 | RLC (HL)

template <Cpu::Register16 rr>
void Cpu::RLC_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::RLC_arr_m1() {
    bool b7 = get_bit<7>(busData);
    u = (busData << 1) | b7;
    mmu.requestWrite(addr, u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b7);
}

template <Cpu::Register16 rr>
void Cpu::RLC_arr_m2() {
    fetch();
}

// e.g. CB 08 | RRC B

template <Cpu::Register8 r>
void Cpu::RRC_r_m0() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    writeRegister8<r>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
    fetch();
}

// e.g. CB 0E | RRC (HL)

template <Cpu::Register16 rr>
void Cpu::RRC_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::RRC_arr_m1() {
    bool b0 = get_bit<0>(busData);
    u = (busData >> 1) | (b0 << 7);
    mmu.requestWrite(addr, u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
}

template <Cpu::Register16 rr>
void Cpu::RRC_arr_m2() {
    fetch();
}

// e.g. CB 10 | RL B

template <Cpu::Register8 r>
void Cpu::RL_r_m0() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | testFlag<Flag::C>();
    writeRegister8<r>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b7);
    fetch();
}

// e.g. CB 16 | RL (HL)

template <Cpu::Register16 rr>
void Cpu::RL_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::RL_arr_m1() {
    bool b7 = get_bit<7>(busData);
    u = (busData << 1) | testFlag<Flag::C>();
    mmu.requestWrite(addr, u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b7);
}

template <Cpu::Register16 rr>
void Cpu::RL_arr_m2() {
    fetch();
}

// e.g. CB 18 | RR B

template <Cpu::Register8 r>
void Cpu::RR_r_m0() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (testFlag<Flag::C>() << 7);
    writeRegister8<r>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
    fetch();
}

// e.g. CB 1E | RR (HL)

template <Cpu::Register16 rr>
void Cpu::RR_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::RR_arr_m1() {
    bool b0 = get_bit<0>(busData);
    u = (busData >> 1) | (testFlag<Flag::C>() << 7);
    mmu.requestWrite(addr, u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
}

template <Cpu::Register16 rr>
void Cpu::RR_arr_m2() {
    fetch();
}

// e.g. CB 28 | SRA B

template <Cpu::Register8 r>
void Cpu::SRA_r_m0() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (u & bit<7>);
    writeRegister8<r>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
    fetch();
}

// e.g. CB 2E | SRA (HL)

template <Cpu::Register16 rr>
void Cpu::SRA_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::SRA_arr_m1() {
    bool b0 = get_bit<0>(busData);
    u = (busData >> 1) | (busData & bit<7>);
    mmu.requestWrite(addr, u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
}

template <Cpu::Register16 rr>
void Cpu::SRA_arr_m2() {
    fetch();
}

// e.g. CB 38 | SRL B

template <Cpu::Register8 r>
void Cpu::SRL_r_m0() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1);
    writeRegister8<r>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
    fetch();
}

// e.g. CB 3E | SRL (HL)

template <Cpu::Register16 rr>
void Cpu::SRL_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::SRL_arr_m1() {
    bool b0 = get_bit<0>(busData);
    u = (busData >> 1);
    mmu.requestWrite(addr, u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b0);
}

template <Cpu::Register16 rr>
void Cpu::SRL_arr_m2() {
    fetch();
}

// e.g. CB 20 | SLA B

template <Cpu::Register8 r>
void Cpu::SLA_r_m0() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = u << 1;
    writeRegister8<r>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b7);
    fetch();
}

// e.g. CB 26 | SLA (HL)

template <Cpu::Register16 rr>
void Cpu::SLA_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::SLA_arr_m1() {
    bool b7 = get_bit<7>(busData);
    u = busData << 1;
    mmu.requestWrite(addr, u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    setFlag<Flag::C>(b7);
}

template <Cpu::Register16 rr>
void Cpu::SLA_arr_m2() {
    fetch();
}

// e.g. CB 30 | SWAP B

template <Cpu::Register8 r>
void Cpu::SWAP_r_m0() {
    u = readRegister8<r>();
    u = ((u & 0x0F) << 4) | ((u & 0xF0) >> 4);
    writeRegister8<r>(u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
    fetch();
}

// e.g. CB 36 | SWAP (HL)

template <Cpu::Register16 rr>
void Cpu::SWAP_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <Cpu::Register16 rr>
void Cpu::SWAP_arr_m1() {
    u = ((busData & 0x0F) << 4) | ((busData & 0xF0) >> 4);
    mmu.requestWrite(addr, u);
    setFlag<Flag::Z>(u == 0);
    resetFlag<(Flag::N)>();
    resetFlag<(Flag::H)>();
    resetFlag<(Flag::C)>();
}

template <Cpu::Register16 rr>
void Cpu::SWAP_arr_m2() {
    fetch();
}

// e.g. CB 40 | BIT 0,B

template <uint8_t n, Cpu::Register8 r>
void Cpu::BIT_r_m0() {
    b = get_bit<n>(readRegister8<r>());
    setFlag<Flag::Z>(b == 0);
    resetFlag<(Flag::N)>();
    setFlag<(Flag::H)>();
    fetch();
}

// e.g. CB 46 | BIT 0,(HL)

template <uint8_t n, Cpu::Register16 rr>
void Cpu::BIT_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::BIT_arr_m1() {
    b = test_bit<n>(busData);
    setFlag<Flag::Z>(b == 0);
    resetFlag<(Flag::N)>();
    setFlag<(Flag::H)>();
    fetch();
}

// e.g. CB 80 | RES 0,B

template <uint8_t n, Cpu::Register8 r>
void Cpu::RES_r_m0() {
    u = readRegister8<r>();
    reset_bit<n>(u);
    writeRegister8<r>(u);
    fetch();
}

// e.g. CB 86 | RES 0,(HL)

template <uint8_t n, Cpu::Register16 rr>
void Cpu::RES_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::RES_arr_m1() {
    reset_bit<n>(busData);
    mmu.requestWrite(addr, busData);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::RES_arr_m2() {
    fetch();
}

// e.g. CB C0 | SET 0,B

template <uint8_t n, Cpu::Register8 r>
void Cpu::SET_r_m0() {
    u = readRegister8<r>();
    set_bit<n>(u);
    writeRegister8<r>(u);
    fetch();
}

// e.g. CB C6 | SET 0,(HL)

template <uint8_t n, Cpu::Register16 rr>
void Cpu::SET_arr_m0() {
    addr = readRegister16<rr>();
    mmu.requestRead(addr);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::SET_arr_m1() {
    set_bit<n>(busData);
    mmu.requestWrite(addr, busData);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::SET_arr_m2() {
    fetch();
}