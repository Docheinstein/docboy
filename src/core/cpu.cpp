#include "cpu.h"
#include "utils/log.h"
#include "utils/binutils.h"
#include <functional>
#include "definitions.h"

Cpu::Cpu(IBus &bus) :
        bus(bus),
        AF(), BC(), DE(), HL(), PC(), SP(),
        IME(),
        halted(),
        mCycles(),
        pendingInterrupt(),
        b(), u(), s(), uu(), lsb(), msb(), addr(),
        currentInstruction(),
        instructions {
	/* 00 */ { &Cpu::NOP_m1 },
	/* 01 */ { &Cpu::LD_rr_uu_m1<Register16::BC>, &Cpu::LD_rr_uu_m2<Register16::BC>, &Cpu::LD_rr_uu_m3<Register16::BC> },
	/* 02 */ { &Cpu::LD_arr_r_m1<Register16::BC, Register8::A>, &Cpu::LD_arr_r_m2<Register16::BC, Register8::A> },
	/* 03 */ { &Cpu::INC_rr_m1<Register16::BC>, &Cpu::INC_rr_m2<Register16::BC> },
	/* 04 */ { &Cpu::INC_r_m1<Register8::B> },
	/* 05 */ { &Cpu::DEC_r_m1<Register8::B> },
	/* 06 */ { &Cpu::LD_r_u_m1<Register8::B>, &Cpu::LD_r_u_m2<Register8::B> },
	/* 07 */ { &Cpu::RLCA_m1 },
	/* 08 */ { &Cpu::LD_ann_rr_m1<Register16::SP>, &Cpu::LD_ann_rr_m2<Register16::SP>, &Cpu::LD_ann_rr_m3<Register16::SP>, &Cpu::LD_ann_rr_m4<Register16::SP>, &Cpu::LD_ann_rr_m5<Register16::SP> },
	/* 09 */ { &Cpu::ADD_rr_rr_m1<Register16::HL, Register16::BC>, &Cpu::ADD_rr_rr_m2<Register16::HL, Register16::BC> },
	/* 0A */ { &Cpu::LD_r_arr_m1<Register8::A, Register16::BC>, &Cpu::LD_r_arr_m2<Register8::A, Register16::BC> },
	/* 0B */ { &Cpu::DEC_rr_m1<Register16::BC>, &Cpu::DEC_rr_m2<Register16::BC> },
	/* 0C */ { &Cpu::INC_r_m1<Register8::C> },
	/* 0D */ { &Cpu::DEC_r_m1<Register8::C> },
	/* 0E */ { &Cpu::LD_r_u_m1<Register8::C>, &Cpu::LD_r_u_m2<Register8::C> },
	/* 0F */ { &Cpu::RRCA_m1 },
	/* 10 */ { &Cpu::STOP_m1 },
	/* 11 */ { &Cpu::LD_rr_uu_m1<Register16::DE>, &Cpu::LD_rr_uu_m2<Register16::DE>, &Cpu::LD_rr_uu_m3<Register16::DE> },
	/* 12 */ { &Cpu::LD_arr_r_m1<Register16::DE, Register8::A>, &Cpu::LD_arr_r_m2<Register16::DE, Register8::A> },
	/* 13 */ { &Cpu::INC_rr_m1<Register16::DE>, &Cpu::INC_rr_m2<Register16::DE> },
	/* 14 */ { &Cpu::INC_r_m1<Register8::D> },
	/* 15 */ { &Cpu::DEC_r_m1<Register8::D> },
	/* 16 */ { &Cpu::LD_r_u_m1<Register8::D>, &Cpu::LD_r_u_m2<Register8::D> },
	/* 17 */ { &Cpu::RLA_m1 },
	/* 18 */ { &Cpu::JR_s_m1, &Cpu::JR_s_m2, &Cpu::JR_s_m3 },
	/* 19 */ { &Cpu::ADD_rr_rr_m1<Register16::HL, Register16::DE>, &Cpu::ADD_rr_rr_m2<Register16::HL, Register16::DE> },
	/* 1A */ { &Cpu::LD_r_arr_m1<Register8::A, Register16::DE>, &Cpu::LD_r_arr_m2<Register8::A, Register16::DE> },
	/* 1B */ { &Cpu::DEC_rr_m1<Register16::DE>, &Cpu::DEC_rr_m2<Register16::DE> },
	/* 1C */ { &Cpu::INC_r_m1<Register8::E> },
	/* 1D */ { &Cpu::DEC_r_m1<Register8::E> },
	/* 1E */ { &Cpu::LD_r_u_m1<Register8::E>, &Cpu::LD_r_u_m2<Register8::E> },
	/* 1F */ { &Cpu::RRA_m1 },
	/* 20 */ { &Cpu::JR_c_s_m1<Flag::Z, false>, &Cpu::JR_c_s_m2<Flag::Z, false>, &Cpu::JR_c_s_m3<Flag::Z, false> },
	/* 21 */ { &Cpu::LD_rr_uu_m1<Register16::HL>, &Cpu::LD_rr_uu_m2<Register16::HL>, &Cpu::LD_rr_uu_m3<Register16::HL> },
	/* 22 */ { &Cpu::LD_arri_r_m1<Register16::HL, Register8::A, 1>, &Cpu::LD_arri_r_m2<Register16::HL, Register8::A, 1> },
	/* 23 */ { &Cpu::INC_rr_m1<Register16::HL>, &Cpu::INC_rr_m2<Register16::HL> },
	/* 24 */ { &Cpu::INC_r_m1<Register8::H> },
	/* 25 */ { &Cpu::DEC_r_m1<Register8::H> },
	/* 26 */ { &Cpu::LD_r_u_m1<Register8::H>, &Cpu::LD_r_u_m2<Register8::H> },
	/* 27 */ { &Cpu::DAA_m1 },
	/* 28 */ { &Cpu::JR_c_s_m1<Flag::Z>, &Cpu::JR_c_s_m2<Flag::Z>, &Cpu::JR_c_s_m3<Flag::Z> },
	/* 29 */ { &Cpu::ADD_rr_rr_m1<Register16::HL, Register16::HL>, &Cpu::ADD_rr_rr_m2<Register16::HL, Register16::HL> },
	/* 2A */ { &Cpu::LD_r_arri_m1<Register8::A, Register16::HL, 1>, &Cpu::LD_r_arri_m2<Register8::A, Register16::HL, 1> },
	/* 2B */ { &Cpu::DEC_rr_m1<Register16::HL>, &Cpu::DEC_rr_m2<Register16::HL> },
	/* 2C */ { &Cpu::INC_r_m1<Register8::L> },
	/* 2D */ { &Cpu::DEC_r_m1<Register8::L> },
	/* 2E */ { &Cpu::LD_r_u_m1<Register8::L>, &Cpu::LD_r_u_m2<Register8::L> },
	/* 2F */ { &Cpu::CPL_m1 },
	/* 30 */ { &Cpu::JR_c_s_m1<Flag::C, false>, &Cpu::JR_c_s_m2<Flag::C, false>, &Cpu::JR_c_s_m3<Flag::C, false> },
	/* 31 */ { &Cpu::LD_rr_uu_m1<Register16::SP>, &Cpu::LD_rr_uu_m2<Register16::SP>, &Cpu::LD_rr_uu_m3<Register16::SP> },
	/* 32 */ { &Cpu::LD_arri_r_m1<Register16::HL, Register8::A, -1>, &Cpu::LD_arri_r_m2<Register16::HL, Register8::A, -1> },
	/* 33 */ { &Cpu::INC_rr_m1<Register16::SP>, &Cpu::INC_rr_m2<Register16::SP> },
	/* 34 */ { &Cpu::INC_arr_m1<Register16::HL>, &Cpu::INC_arr_m2<Register16::HL>, &Cpu::INC_arr_m3<Register16::HL> },
	/* 35 */ { &Cpu::DEC_arr_m1<Register16::HL>, &Cpu::DEC_arr_m2<Register16::HL>, &Cpu::DEC_arr_m3<Register16::HL> },
	/* 36 */ { &Cpu::LD_arr_u_m1<Register16::HL>, &Cpu::LD_arr_u_m2<Register16::HL>, &Cpu::LD_arr_u_m3<Register16::HL> },
	/* 37 */ { &Cpu::SCF_m1 },
	/* 38 */ { &Cpu::JR_c_s_m1<Flag::C>, &Cpu::JR_c_s_m2<Flag::C>, &Cpu::JR_c_s_m3<Flag::C> },
	/* 39 */ { &Cpu::ADD_rr_rr_m1<Register16::HL, Register16::SP>, &Cpu::ADD_rr_rr_m2<Register16::HL, Register16::SP> },
	/* 3A */ { &Cpu::LD_r_arri_m1<Register8::A, Register16::HL, -1>, &Cpu::LD_r_arri_m2<Register8::A, Register16::HL, -1> },
	/* 3B */ { &Cpu::DEC_rr_m1<Register16::SP>, &Cpu::DEC_rr_m2<Register16::SP> },
	/* 3C */ { &Cpu::INC_r_m1<Register8::A> },
	/* 3D */ { &Cpu::DEC_r_m1<Register8::A> },
	/* 3E */ { &Cpu::LD_r_u_m1<Register8::A>, &Cpu::LD_r_u_m2<Register8::A> },
	/* 3F */ { &Cpu::CCF_m1 },
	/* 40 */ { &Cpu::LD_r_r_m1<Register8::B, Register8::B> },
	/* 41 */ { &Cpu::LD_r_r_m1<Register8::B, Register8::C> },
	/* 42 */ { &Cpu::LD_r_r_m1<Register8::B, Register8::D> },
	/* 43 */ { &Cpu::LD_r_r_m1<Register8::B, Register8::E> },
	/* 44 */ { &Cpu::LD_r_r_m1<Register8::B, Register8::H> },
	/* 45 */ { &Cpu::LD_r_r_m1<Register8::B, Register8::L> },
	/* 46 */ { &Cpu::LD_r_arr_m1<Register8::B, Register16::HL>, &Cpu::LD_r_arr_m2<Register8::B, Register16::HL> },
	/* 47 */ { &Cpu::LD_r_r_m1<Register8::B, Register8::A> },
	/* 48 */ { &Cpu::LD_r_r_m1<Register8::C, Register8::B> },
	/* 49 */ { &Cpu::LD_r_r_m1<Register8::C, Register8::C> },
	/* 4A */ { &Cpu::LD_r_r_m1<Register8::C, Register8::D> },
	/* 4B */ { &Cpu::LD_r_r_m1<Register8::C, Register8::E> },
	/* 4C */ { &Cpu::LD_r_r_m1<Register8::C, Register8::H> },
	/* 4D */ { &Cpu::LD_r_r_m1<Register8::C, Register8::L> },
	/* 4E */ { &Cpu::LD_r_arr_m1<Register8::C, Register16::HL>, &Cpu::LD_r_arr_m2<Register8::C, Register16::HL> },
	/* 4F */ { &Cpu::LD_r_r_m1<Register8::C, Register8::A> },
	/* 50 */ { &Cpu::LD_r_r_m1<Register8::D, Register8::B> },
	/* 51 */ { &Cpu::LD_r_r_m1<Register8::D, Register8::C> },
	/* 52 */ { &Cpu::LD_r_r_m1<Register8::D, Register8::D> },
	/* 53 */ { &Cpu::LD_r_r_m1<Register8::D, Register8::E> },
	/* 54 */ { &Cpu::LD_r_r_m1<Register8::D, Register8::H> },
	/* 55 */ { &Cpu::LD_r_r_m1<Register8::D, Register8::L> },
	/* 56 */ { &Cpu::LD_r_arr_m1<Register8::D, Register16::HL>, &Cpu::LD_r_arr_m2<Register8::D, Register16::HL> },
	/* 57 */ { &Cpu::LD_r_r_m1<Register8::D, Register8::A> },
	/* 58 */ { &Cpu::LD_r_r_m1<Register8::E, Register8::B> },
	/* 59 */ { &Cpu::LD_r_r_m1<Register8::E, Register8::C> },
	/* 5A */ { &Cpu::LD_r_r_m1<Register8::E, Register8::D> },
	/* 5B */ { &Cpu::LD_r_r_m1<Register8::E, Register8::E> },
	/* 5C */ { &Cpu::LD_r_r_m1<Register8::E, Register8::H> },
	/* 5D */ { &Cpu::LD_r_r_m1<Register8::E, Register8::L> },
	/* 5E */ { &Cpu::LD_r_arr_m1<Register8::E, Register16::HL>, &Cpu::LD_r_arr_m2<Register8::E, Register16::HL> },
	/* 5F */ { &Cpu::LD_r_r_m1<Register8::E, Register8::A> },
	/* 60 */ { &Cpu::LD_r_r_m1<Register8::H, Register8::B> },
	/* 61 */ { &Cpu::LD_r_r_m1<Register8::H, Register8::C> },
	/* 62 */ { &Cpu::LD_r_r_m1<Register8::H, Register8::D> },
	/* 63 */ { &Cpu::LD_r_r_m1<Register8::H, Register8::E> },
	/* 64 */ { &Cpu::LD_r_r_m1<Register8::H, Register8::H> },
	/* 65 */ { &Cpu::LD_r_r_m1<Register8::H, Register8::L> },
	/* 66 */ { &Cpu::LD_r_arr_m1<Register8::H, Register16::HL>, &Cpu::LD_r_arr_m2<Register8::H, Register16::HL> },
	/* 67 */ { &Cpu::LD_r_r_m1<Register8::H, Register8::A> },
	/* 68 */ { &Cpu::LD_r_r_m1<Register8::L, Register8::B> },
	/* 69 */ { &Cpu::LD_r_r_m1<Register8::L, Register8::C> },
	/* 6A */ { &Cpu::LD_r_r_m1<Register8::L, Register8::D> },
	/* 6B */ { &Cpu::LD_r_r_m1<Register8::L, Register8::E> },
	/* 6C */ { &Cpu::LD_r_r_m1<Register8::L, Register8::H> },
	/* 6D */ { &Cpu::LD_r_r_m1<Register8::L, Register8::L> },
	/* 6E */ { &Cpu::LD_r_arr_m1<Register8::L, Register16::HL>, &Cpu::LD_r_arr_m2<Register8::L, Register16::HL> },
	/* 6F */ { &Cpu::LD_r_r_m1<Register8::L, Register8::A> },
	/* 70 */ { &Cpu::LD_arr_r_m1<Register16::HL, Register8::B>, &Cpu::LD_arr_r_m2<Register16::HL, Register8::B> },
	/* 71 */ { &Cpu::LD_arr_r_m1<Register16::HL, Register8::C>, &Cpu::LD_arr_r_m2<Register16::HL, Register8::C> },
	/* 72 */ { &Cpu::LD_arr_r_m1<Register16::HL, Register8::D>, &Cpu::LD_arr_r_m2<Register16::HL, Register8::D> },
	/* 73 */ { &Cpu::LD_arr_r_m1<Register16::HL, Register8::E>, &Cpu::LD_arr_r_m2<Register16::HL, Register8::E> },
	/* 74 */ { &Cpu::LD_arr_r_m1<Register16::HL, Register8::H>, &Cpu::LD_arr_r_m2<Register16::HL, Register8::H> },
	/* 75 */ { &Cpu::LD_arr_r_m1<Register16::HL, Register8::L>, &Cpu::LD_arr_r_m2<Register16::HL, Register8::L> },
	/* 76 */ { &Cpu::HALT_m1 },
	/* 77 */ { &Cpu::LD_arr_r_m1<Register16::HL, Register8::A>, &Cpu::LD_arr_r_m2<Register16::HL, Register8::A> },
	/* 78 */ { &Cpu::LD_r_r_m1<Register8::A, Register8::B> },
	/* 79 */ { &Cpu::LD_r_r_m1<Register8::A, Register8::C> },
	/* 7A */ { &Cpu::LD_r_r_m1<Register8::A, Register8::D> },
	/* 7B */ { &Cpu::LD_r_r_m1<Register8::A, Register8::E> },
	/* 7C */ { &Cpu::LD_r_r_m1<Register8::A, Register8::H> },
	/* 7D */ { &Cpu::LD_r_r_m1<Register8::A, Register8::L> },
	/* 7E */ { &Cpu::LD_r_arr_m1<Register8::A, Register16::HL>, &Cpu::LD_r_arr_m2<Register8::A, Register16::HL> },
	/* 7F */ { &Cpu::LD_r_r_m1<Register8::A, Register8::A> },
	/* 80 */ { &Cpu::ADD_r_m1<Register8::B> },
	/* 81 */ { &Cpu::ADD_r_m1<Register8::C> },
	/* 82 */ { &Cpu::ADD_r_m1<Register8::D> },
	/* 83 */ { &Cpu::ADD_r_m1<Register8::E> },
	/* 84 */ { &Cpu::ADD_r_m1<Register8::H> },
	/* 85 */ { &Cpu::ADD_r_m1<Register8::L> },
	/* 86 */ { &Cpu::ADD_arr_m1<Register16::HL>, &Cpu::ADD_arr_m2<Register16::HL> },
	/* 87 */ { &Cpu::ADD_r_m1<Register8::A> },
	/* 88 */ { &Cpu::ADC_r_m1<Register8::B> },
	/* 89 */ { &Cpu::ADC_r_m1<Register8::C> },
	/* 8A */ { &Cpu::ADC_r_m1<Register8::D> },
	/* 8B */ { &Cpu::ADC_r_m1<Register8::E> },
	/* 8C */ { &Cpu::ADC_r_m1<Register8::H> },
	/* 8D */ { &Cpu::ADC_r_m1<Register8::L> },
	/* 8E */ { &Cpu::ADC_arr_m1<Register16::HL>, &Cpu::ADC_arr_m2<Register16::HL> },
	/* 8F */ { &Cpu::ADC_r_m1<Register8::A> },
	/* 90 */ { &Cpu::SUB_r_m1<Register8::B> },
	/* 91 */ { &Cpu::SUB_r_m1<Register8::C> },
	/* 92 */ { &Cpu::SUB_r_m1<Register8::D> },
	/* 93 */ { &Cpu::SUB_r_m1<Register8::E> },
	/* 94 */ { &Cpu::SUB_r_m1<Register8::H> },
	/* 95 */ { &Cpu::SUB_r_m1<Register8::L> },
	/* 96 */ { &Cpu::SUB_arr_m1<Register16::HL>, &Cpu::SUB_arr_m2<Register16::HL> },
	/* 97 */ { &Cpu::SUB_r_m1<Register8::A> },
	/* 98 */ { &Cpu::SBC_r_m1<Register8::B> },
	/* 99 */ { &Cpu::SBC_r_m1<Register8::C> },
	/* 9A */ { &Cpu::SBC_r_m1<Register8::D> },
	/* 9B */ { &Cpu::SBC_r_m1<Register8::E> },
	/* 9C */ { &Cpu::SBC_r_m1<Register8::H> },
	/* 9D */ { &Cpu::SBC_r_m1<Register8::L> },
	/* 9E */ { &Cpu::SBC_arr_m1<Register16::HL>, &Cpu::SBC_arr_m2<Register16::HL>  },
	/* 9F */ { &Cpu::SBC_r_m1<Register8::A> },
	/* A0 */ { &Cpu::AND_r_m1<Register8::B> },
	/* A1 */ { &Cpu::AND_r_m1<Register8::C> },
	/* A2 */ { &Cpu::AND_r_m1<Register8::D> },
	/* A3 */ { &Cpu::AND_r_m1<Register8::E> },
	/* A4 */ { &Cpu::AND_r_m1<Register8::H> },
	/* A5 */ { &Cpu::AND_r_m1<Register8::L> },
	/* A6 */ { &Cpu::AND_arr_m1<Register16::HL>, &Cpu::AND_arr_m2<Register16::HL> },
	/* A7 */ { &Cpu::AND_r_m1<Register8::A> },
	/* A8 */ { &Cpu::XOR_r_m1<Register8::B> },
	/* A9 */ { &Cpu::XOR_r_m1<Register8::C> },
	/* AA */ { &Cpu::XOR_r_m1<Register8::D> },
	/* AB */ { &Cpu::XOR_r_m1<Register8::E>  },
	/* AC */ { &Cpu::XOR_r_m1<Register8::H>  },
	/* AD */ { &Cpu::XOR_r_m1<Register8::L>  },
	/* AE */ { &Cpu::XOR_arr_m1<Register16::HL>, &Cpu::XOR_arr_m2<Register16::HL> },
	/* AF */ { &Cpu::XOR_r_m1<Register8::A> },
	/* B0 */ { &Cpu::OR_r_m1<Register8::B> },
	/* B1 */ { &Cpu::OR_r_m1<Register8::C> },
	/* B2 */ { &Cpu::OR_r_m1<Register8::D> },
	/* B3 */ { &Cpu::OR_r_m1<Register8::E> },
	/* B4 */ { &Cpu::OR_r_m1<Register8::H> },
	/* B5 */ { &Cpu::OR_r_m1<Register8::L> },
	/* B6 */ { &Cpu::OR_arr_m1<Register16::HL>, &Cpu::OR_arr_m2<Register16::HL> },
	/* B7 */ { &Cpu::OR_r_m1<Register8::A> },
	/* B8 */ { &Cpu::CP_r_m1<Register8::B> },
	/* B9 */ { &Cpu::CP_r_m1<Register8::C> },
	/* BA */ { &Cpu::CP_r_m1<Register8::D> },
	/* BB */ { &Cpu::CP_r_m1<Register8::E> },
	/* BC */ { &Cpu::CP_r_m1<Register8::H> },
	/* BD */ { &Cpu::CP_r_m1<Register8::L> },
	/* BE */ { &Cpu::CP_arr_m1<Register16::HL>, &Cpu::CP_arr_m2<Register16::HL> },
	/* BF */ { &Cpu::CP_r_m1<Register8::A> },
	/* C0 */ { &Cpu::RET_c_uu_m1<Flag::Z, false>, &Cpu::RET_c_uu_m2<Flag::Z, false>, &Cpu::RET_c_uu_m3<Flag::Z, false>,
	            &Cpu::RET_c_uu_m4<Flag::Z, false>, &Cpu::RET_c_uu_m5<Flag::Z, false> },
	/* C1 */ { &Cpu::POP_rr_m1<Register16::BC>, &Cpu::POP_rr_m2<Register16::BC>, &Cpu::POP_rr_m3<Register16::BC> },
	/* C2 */ { &Cpu::JP_c_uu_m1<Flag::Z, false>, &Cpu::JP_c_uu_m2<Flag::Z, false>, &Cpu::JP_c_uu_m3<Flag::Z, false>, &Cpu::JP_c_uu_m4<Flag::Z, false> },
	/* C3 */ { &Cpu::JP_uu_m1, &Cpu::JP_uu_m2, &Cpu::JP_uu_m3, &Cpu::JP_uu_m4 },
	/* C4 */ { &Cpu::CALL_c_uu_m1<Flag::Z, false>, &Cpu::CALL_c_uu_m2<Flag::Z, false>, &Cpu::CALL_c_uu_m3<Flag::Z, false>,
	            &Cpu::CALL_c_uu_m4<Flag::Z, false>, &Cpu::CALL_c_uu_m5<Flag::Z, false>, &Cpu::CALL_c_uu_m6<Flag::Z, false>, },
	/* C5 */ { &Cpu::PUSH_rr_m1<Register16::BC>, &Cpu::PUSH_rr_m2<Register16::BC>, &Cpu::PUSH_rr_m3<Register16::BC>, &Cpu::PUSH_rr_m4<Register16::BC> },
	/* C6 */ { &Cpu::ADD_u_m1, &Cpu::ADD_u_m2 },
	/* C7 */ { &Cpu::RST_m1<0x00>, &Cpu::RST_m2<0x00>, &Cpu::RST_m3<0x00>, &Cpu::RST_m4<0x00> },
	/* C8 */ { &Cpu::RET_c_uu_m1<Flag::Z>, &Cpu::RET_c_uu_m2<Flag::Z>, &Cpu::RET_c_uu_m3<Flag::Z>,
	            &Cpu::RET_c_uu_m4<Flag::Z>, &Cpu::RET_c_uu_m5<Flag::Z> },
	/* C9 */ { &Cpu::RET_uu_m1, &Cpu::RET_uu_m2, &Cpu::RET_uu_m3, &Cpu::RET_uu_m4 },
	/* CA */ { &Cpu::JP_c_uu_m1<Flag::Z>, &Cpu::JP_c_uu_m2<Flag::Z>, &Cpu::JP_c_uu_m3<Flag::Z>, &Cpu::JP_c_uu_m4<Flag::Z> },
	/* CB */ { &Cpu::CB_m1 },
	/* CC */ { &Cpu::CALL_c_uu_m1<Flag::Z>, &Cpu::CALL_c_uu_m2<Flag::Z>, &Cpu::CALL_c_uu_m3<Flag::Z>,
	            &Cpu::CALL_c_uu_m4<Flag::Z>, &Cpu::CALL_c_uu_m5<Flag::Z>, &Cpu::CALL_c_uu_m6<Flag::Z>, },
	/* CD */ { &Cpu::CALL_uu_m1, &Cpu::CALL_uu_m2, &Cpu::CALL_uu_m3,
	            &Cpu::CALL_uu_m4, &Cpu::CALL_uu_m5, &Cpu::CALL_uu_m6, },
	/* CE */ { &Cpu::ADC_u_m1, &Cpu::ADC_u_m2 },
	/* CF */ { &Cpu::RST_m1<0x08>, &Cpu::RST_m2<0x08>, &Cpu::RST_m3<0x08>, &Cpu::RST_m4<0x08> },
	/* D0 */ { &Cpu::RET_c_uu_m1<Flag::C, false>, &Cpu::RET_c_uu_m2<Flag::C, false>, &Cpu::RET_c_uu_m3<Flag::C, false>,
	            &Cpu::RET_c_uu_m4<Flag::C, false>, &Cpu::RET_c_uu_m5<Flag::C, false> },
	/* D1 */ { &Cpu::POP_rr_m1<Register16::DE>, &Cpu::POP_rr_m2<Register16::DE>, &Cpu::POP_rr_m3<Register16::DE> },
	/* D2 */ { &Cpu::JP_c_uu_m1<Flag::C, false>, &Cpu::JP_c_uu_m2<Flag::C, false>, &Cpu::JP_c_uu_m3<Flag::C, false>, &Cpu::JP_c_uu_m4<Flag::C, false> },
	/* D3 */ { &Cpu::invalidInstruction },
	/* D4 */ { &Cpu::CALL_c_uu_m1<Flag::C, false>, &Cpu::CALL_c_uu_m2<Flag::C, false>, &Cpu::CALL_c_uu_m3<Flag::C, false>,
	            &Cpu::CALL_c_uu_m4<Flag::C, false>, &Cpu::CALL_c_uu_m5<Flag::C, false>, &Cpu::CALL_c_uu_m6<Flag::C, false>, },
	/* D5 */ { &Cpu::PUSH_rr_m1<Register16::DE>, &Cpu::PUSH_rr_m2<Register16::DE>, &Cpu::PUSH_rr_m3<Register16::DE>, &Cpu::PUSH_rr_m4<Register16::DE> },
	/* D6 */ { &Cpu::SUB_u_m1, &Cpu::SUB_u_m2 },
	/* D7 */ { &Cpu::RST_m1<0x10>, &Cpu::RST_m2<0x10>, &Cpu::RST_m3<0x10>, &Cpu::RST_m4<0x10> },
	/* D8 */ { &Cpu::RET_c_uu_m1<Flag::C>, &Cpu::RET_c_uu_m2<Flag::C>, &Cpu::RET_c_uu_m3<Flag::C>,
	            &Cpu::RET_c_uu_m4<Flag::C>, &Cpu::RET_c_uu_m5<Flag::C> },
	/* D9 */ { &Cpu::RETI_uu_m1, &Cpu::RETI_uu_m2, &Cpu::RETI_uu_m3, &Cpu::RETI_uu_m4 },
	/* DA */ { &Cpu::JP_c_uu_m1<Flag::C>, &Cpu::JP_c_uu_m2<Flag::C>, &Cpu::JP_c_uu_m3<Flag::C>, &Cpu::JP_c_uu_m4<Flag::C> },
	/* DB */ { &Cpu::invalidInstruction },
	/* DC */ { &Cpu::CALL_c_uu_m1<Flag::C>, &Cpu::CALL_c_uu_m2<Flag::C>, &Cpu::CALL_c_uu_m3<Flag::C>,
	            &Cpu::CALL_c_uu_m4<Flag::C>, &Cpu::CALL_c_uu_m5<Flag::C>, &Cpu::CALL_c_uu_m6<Flag::C>, },
	/* DD */ { &Cpu::invalidInstruction },
	/* DE */ { &Cpu::SBC_u_m1, &Cpu::SBC_u_m2 },
	/* DF */ { &Cpu::RST_m1<0x18>, &Cpu::RST_m2<0x18>, &Cpu::RST_m3<0x18>, &Cpu::RST_m4<0x18> },
	/* E0 */ { &Cpu::LDH_an_r_m1<Register8::A>, &Cpu::LDH_an_r_m2<Register8::A>, &Cpu::LDH_an_r_m3<Register8::A> },
	/* E1 */ { &Cpu::POP_rr_m1<Register16::HL>, &Cpu::POP_rr_m2<Register16::HL>, &Cpu::POP_rr_m3<Register16::HL> },
	/* E2 */ { &Cpu::LDH_ar_r_m1<Register8::C, Register8::A>, &Cpu::LDH_ar_r_m2<Register8::C, Register8::A> },
	/* E3 */ { &Cpu::invalidInstruction },
	/* E4 */ { &Cpu::invalidInstruction },
	/* E5 */ { &Cpu::PUSH_rr_m1<Register16::HL>, &Cpu::PUSH_rr_m2<Register16::HL>, &Cpu::PUSH_rr_m3<Register16::HL>, &Cpu::PUSH_rr_m4<Register16::HL> },
	/* E6 */ { &Cpu::AND_u_m1, &Cpu::AND_u_m2 },
	/* E7 */ { &Cpu::RST_m1<0x20>, &Cpu::RST_m2<0x20>, &Cpu::RST_m3<0x20>, &Cpu::RST_m4<0x20> },
	/* E8 */ { &Cpu::ADD_rr_s_m1<Register16::SP>, &Cpu::ADD_rr_s_m2<Register16::SP>, &Cpu::ADD_rr_s_m3<Register16::SP>, &Cpu::ADD_rr_s_m4<Register16::SP> },
	/* E9 */ { &Cpu::JP_rr_m1<Register16::HL> },
	/* EA */ { &Cpu::LD_ann_r_m1<Register8::A>, &Cpu::LD_ann_r_m2<Register8::A>, &Cpu::LD_ann_r_m3<Register8::A>, &Cpu::LD_ann_r_m4<Register8::A> },
	/* EB */ { &Cpu::invalidInstruction },
	/* EC */ { &Cpu::invalidInstruction },
	/* ED */ { &Cpu::invalidInstruction },
	/* EE */ { &Cpu::XOR_u_m1, &Cpu::XOR_u_m2 },
	/* EF */ { &Cpu::RST_m1<0x28>, &Cpu::RST_m2<0x28>, &Cpu::RST_m3<0x28>, &Cpu::RST_m4<0x28> },
	/* F0 */ { &Cpu::LDH_r_an_m1<Register8::A>, &Cpu::LDH_r_an_m2<Register8::A>, &Cpu::LDH_r_an_m3<Register8::A> },
	/* F1 */ { &Cpu::POP_rr_m1<Register16::AF>, &Cpu::POP_rr_m2<Register16::AF>, &Cpu::POP_rr_m3<Register16::AF> },
	/* F2 */ { &Cpu::LDH_r_ar_m1<Register8::A, Register8::C>, &Cpu::LDH_r_ar_m2<Register8::A, Register8::C> },
	/* F3 */ { &Cpu::DI_m1 },
	/* F4 */ { &Cpu::invalidInstruction },
	/* F5 */ { &Cpu::PUSH_rr_m1<Register16::AF>, &Cpu::PUSH_rr_m2<Register16::AF>, &Cpu::PUSH_rr_m3<Register16::AF>, &Cpu::PUSH_rr_m4<Register16::AF> },
	/* F6 */ { &Cpu::OR_u_m1, &Cpu::OR_u_m2 },
	/* F7 */ { &Cpu::RST_m1<0x30>, &Cpu::RST_m2<0x30>, &Cpu::RST_m3<0x30>, &Cpu::RST_m4<0x30> },
	/* F8 */ { &Cpu::LD_rr_rrs_m1<Register16::HL, Register16::SP>, &Cpu::LD_rr_rrs_m2<Register16::HL, Register16::SP>, &Cpu::LD_rr_rrs_m3<Register16::HL, Register16::SP> },
	/* F9 */ { &Cpu::LD_rr_rr_m1<Register16::SP, Register16::HL>, &Cpu::LD_rr_rr_m2<Register16::SP, Register16::HL> },
	/* FA */ { &Cpu::LD_r_ann_m1<Register8::A>, &Cpu::LD_r_ann_m2<Register8::A>, &Cpu::LD_r_ann_m3<Register8::A>, &Cpu::LD_r_ann_m4<Register8::A> },
	/* FB */ { &Cpu::EI_m1 },
	/* FC */ { &Cpu::invalidInstruction },
	/* FD */ { &Cpu::invalidInstruction },
	/* FE */ { &Cpu::CP_u_m1, &Cpu::CP_u_m2 },
	/* FF */ { &Cpu::RST_m1<0x38>, &Cpu::RST_m2<0x38>, &Cpu::RST_m3<0x38>, &Cpu::RST_m4<0x38> },
    },
    instructions_cb {
	/* 00 */ { &Cpu::RLC_r_m1<Register8::B> },
	/* 01 */ { &Cpu::RLC_r_m1<Register8::C> },
	/* 02 */ { &Cpu::RLC_r_m1<Register8::D> },
	/* 03 */ { &Cpu::RLC_r_m1<Register8::E> },
	/* 04 */ { &Cpu::RLC_r_m1<Register8::H> },
	/* 05 */ { &Cpu::RLC_r_m1<Register8::L> },
	/* 06 */ { &Cpu::RLC_arr_m1<Register16::HL>, &Cpu::RLC_arr_m2<Register16::HL>, &Cpu::RLC_arr_m3<Register16::HL> },
	/* 07 */ { &Cpu::RLC_r_m1<Register8::A> },
	/* 08 */ { &Cpu::RRC_r_m1<Register8::B> },
	/* 09 */ { &Cpu::RRC_r_m1<Register8::C> },
	/* 0A */ { &Cpu::RRC_r_m1<Register8::D> },
	/* 0B */ { &Cpu::RRC_r_m1<Register8::E> },
	/* 0C */ { &Cpu::RRC_r_m1<Register8::H> },
	/* 0D */ { &Cpu::RRC_r_m1<Register8::L> },
	/* 0E */ { &Cpu::RRC_arr_m1<Register16::HL>, &Cpu::RRC_arr_m2<Register16::HL>, &Cpu::RRC_arr_m3<Register16::HL> },
	/* 0F */ { &Cpu::RRC_r_m1<Register8::A> },
	/* 10 */ { &Cpu::RL_r_m1<Register8::B> },
	/* 11 */ { &Cpu::RL_r_m1<Register8::C> },
	/* 12 */ { &Cpu::RL_r_m1<Register8::D> },
	/* 13 */ { &Cpu::RL_r_m1<Register8::E> },
	/* 14 */ { &Cpu::RL_r_m1<Register8::H> },
	/* 15 */ { &Cpu::RL_r_m1<Register8::L> },
	/* 16 */ { &Cpu::RL_arr_m1<Register16::HL>, &Cpu::RL_arr_m2<Register16::HL>, &Cpu::RL_arr_m3<Register16::HL> },
	/* 17 */ { &Cpu::RL_r_m1<Register8::A> },
	/* 18 */ { &Cpu::RR_r_m1<Register8::B> },
	/* 19 */ { &Cpu::RR_r_m1<Register8::C> },
	/* 1A */ { &Cpu::RR_r_m1<Register8::D> },
	/* 1B */ { &Cpu::RR_r_m1<Register8::E> },
	/* 1C */ { &Cpu::RR_r_m1<Register8::H> },
	/* 1D */ { &Cpu::RR_r_m1<Register8::L> },
	/* 1E */ { &Cpu::RR_arr_m1<Register16::HL>, &Cpu::RR_arr_m2<Register16::HL>, &Cpu::RR_arr_m3<Register16::HL> },
	/* 1F */ { &Cpu::RR_r_m1<Register8::A> },
	/* 20 */ { &Cpu::SLA_r_m1<Register8::B> },
	/* 21 */ { &Cpu::SLA_r_m1<Register8::C> },
	/* 22 */ { &Cpu::SLA_r_m1<Register8::D> },
	/* 23 */ { &Cpu::SLA_r_m1<Register8::E> },
	/* 24 */ { &Cpu::SLA_r_m1<Register8::H> },
	/* 25 */ { &Cpu::SLA_r_m1<Register8::L> },
	/* 26 */ { &Cpu::SLA_arr_m1<Register16::HL>, &Cpu::SLA_arr_m2<Register16::HL>, &Cpu::SLA_arr_m3<Register16::HL>  },
	/* 27 */ { &Cpu::SLA_r_m1<Register8::A> },
	/* 28 */ { &Cpu::SRA_r_m1<Register8::B> },
	/* 29 */ { &Cpu::SRA_r_m1<Register8::C> },
	/* 2A */ { &Cpu::SRA_r_m1<Register8::D> },
	/* 2B */ { &Cpu::SRA_r_m1<Register8::E> },
	/* 2C */ { &Cpu::SRA_r_m1<Register8::H> },
	/* 2D */ { &Cpu::SRA_r_m1<Register8::L> },
	/* 2E */ { &Cpu::SRA_arr_m1<Register16::HL>, &Cpu::SRA_arr_m2<Register16::HL>, &Cpu::SRA_arr_m3<Register16::HL> },
	/* 2F */ { &Cpu::SRA_r_m1<Register8::A> },
	/* 30 */ { &Cpu::SWAP_r_m1<Register8::B> },
	/* 31 */ { &Cpu::SWAP_r_m1<Register8::C> },
	/* 32 */ { &Cpu::SWAP_r_m1<Register8::D> },
	/* 33 */ { &Cpu::SWAP_r_m1<Register8::E> },
	/* 34 */ { &Cpu::SWAP_r_m1<Register8::H> },
	/* 35 */ { &Cpu::SWAP_r_m1<Register8::L> },
	/* 36 */ { &Cpu::SWAP_arr_m1<Register16::HL>, &Cpu::SWAP_arr_m2<Register16::HL>, &Cpu::SWAP_arr_m3<Register16::HL> },
	/* 37 */ { &Cpu::SWAP_r_m1<Register8::A> },
	/* 38 */ { &Cpu::SRL_r_m1<Register8::B> },
	/* 39 */ { &Cpu::SRL_r_m1<Register8::C> },
	/* 3A */ { &Cpu::SRL_r_m1<Register8::D> },
	/* 3B */ { &Cpu::SRL_r_m1<Register8::E> },
	/* 3C */ { &Cpu::SRL_r_m1<Register8::H> },
	/* 3D */ { &Cpu::SRL_r_m1<Register8::L> },
	/* 3E */ { &Cpu::SRL_arr_m1<Register16::HL>, &Cpu::SRL_arr_m2<Register16::HL>, &Cpu::SRL_arr_m3<Register16::HL> },
	/* 3F */ { &Cpu::SRL_r_m1<Register8::A> },
	/* 40 */ { &Cpu::BIT_r_m1<0, Register8::B> },
	/* 41 */ { &Cpu::BIT_r_m1<0, Register8::C> },
	/* 42 */ { &Cpu::BIT_r_m1<0, Register8::D> },
	/* 43 */ { &Cpu::BIT_r_m1<0, Register8::E> },
	/* 44 */ { &Cpu::BIT_r_m1<0, Register8::H> },
	/* 45 */ { &Cpu::BIT_r_m1<0, Register8::L> },
	/* 46 */ { &Cpu::BIT_arr_m1<0, Register16::HL>, &Cpu::BIT_arr_m2<0, Register16::HL> },
	/* 47 */ { &Cpu::BIT_r_m1<0, Register8::A> },
	/* 48 */ { &Cpu::BIT_r_m1<1, Register8::B> },
	/* 49 */ { &Cpu::BIT_r_m1<1, Register8::C> },
	/* 4A */ { &Cpu::BIT_r_m1<1, Register8::D> },
	/* 4B */ { &Cpu::BIT_r_m1<1, Register8::E> },
	/* 4C */ { &Cpu::BIT_r_m1<1, Register8::H> },
	/* 4D */ { &Cpu::BIT_r_m1<1, Register8::L> },
	/* 4E */ { &Cpu::BIT_arr_m1<1, Register16::HL>, &Cpu::BIT_arr_m2<1, Register16::HL> },
	/* 4F */ { &Cpu::BIT_r_m1<1, Register8::A> },
	/* 50 */ { &Cpu::BIT_r_m1<2, Register8::B> },
	/* 51 */ { &Cpu::BIT_r_m1<2, Register8::C> },
	/* 52 */ { &Cpu::BIT_r_m1<2, Register8::D> },
	/* 53 */ { &Cpu::BIT_r_m1<2, Register8::E> },
	/* 54 */ { &Cpu::BIT_r_m1<2, Register8::H> },
	/* 55 */ { &Cpu::BIT_r_m1<2, Register8::L> },
	/* 56 */ { &Cpu::BIT_arr_m1<2, Register16::HL>, &Cpu::BIT_arr_m2<2, Register16::HL> },
	/* 57 */ { &Cpu::BIT_r_m1<2, Register8::A> },
	/* 58 */ { &Cpu::BIT_r_m1<3, Register8::B> },
	/* 59 */ { &Cpu::BIT_r_m1<3, Register8::C> },
	/* 5A */ { &Cpu::BIT_r_m1<3, Register8::D> },
	/* 5B */ { &Cpu::BIT_r_m1<3, Register8::E> },
	/* 5C */ { &Cpu::BIT_r_m1<3, Register8::H> },
	/* 5D */ { &Cpu::BIT_r_m1<3, Register8::L> },
	/* 5E */ { &Cpu::BIT_arr_m1<3, Register16::HL>, &Cpu::BIT_arr_m2<3, Register16::HL> },
	/* 5F */ { &Cpu::BIT_r_m1<3, Register8::A> },
	/* 60 */ { &Cpu::BIT_r_m1<4, Register8::B> },
	/* 61 */ { &Cpu::BIT_r_m1<4, Register8::C> },
	/* 62 */ { &Cpu::BIT_r_m1<4, Register8::D> },
	/* 63 */ { &Cpu::BIT_r_m1<4, Register8::E> },
	/* 64 */ { &Cpu::BIT_r_m1<4, Register8::H> },
	/* 65 */ { &Cpu::BIT_r_m1<4, Register8::L> },
	/* 66 */ { &Cpu::BIT_arr_m1<4, Register16::HL>, &Cpu::BIT_arr_m2<4, Register16::HL> },
	/* 67 */ { &Cpu::BIT_r_m1<4, Register8::A> },
	/* 68 */ { &Cpu::BIT_r_m1<5, Register8::B> },
	/* 69 */ { &Cpu::BIT_r_m1<5, Register8::C> },
	/* 6A */ { &Cpu::BIT_r_m1<5, Register8::D> },
	/* 6B */ { &Cpu::BIT_r_m1<5, Register8::E> },
	/* 6C */ { &Cpu::BIT_r_m1<5, Register8::H> },
	/* 6D */ { &Cpu::BIT_r_m1<5, Register8::L> },
	/* 6E */ { &Cpu::BIT_arr_m1<5, Register16::HL>, &Cpu::BIT_arr_m2<5, Register16::HL> },
	/* 6F */ { &Cpu::BIT_r_m1<5, Register8::A> },
	/* 70 */ { &Cpu::BIT_r_m1<6, Register8::B> },
	/* 71 */ { &Cpu::BIT_r_m1<6, Register8::C> },
	/* 72 */ { &Cpu::BIT_r_m1<6, Register8::D> },
	/* 73 */ { &Cpu::BIT_r_m1<6, Register8::E> },
	/* 74 */ { &Cpu::BIT_r_m1<6, Register8::H> },
	/* 75 */ { &Cpu::BIT_r_m1<6, Register8::L> },
	/* 76 */ { &Cpu::BIT_arr_m1<6, Register16::HL>, &Cpu::BIT_arr_m2<6, Register16::HL> },
	/* 77 */ { &Cpu::BIT_r_m1<6, Register8::A> },
	/* 78 */ { &Cpu::BIT_r_m1<7, Register8::B> },
	/* 79 */ { &Cpu::BIT_r_m1<7, Register8::C> },
	/* 7A */ { &Cpu::BIT_r_m1<7, Register8::D> },
	/* 7B */ { &Cpu::BIT_r_m1<7, Register8::E> },
	/* 7C */ { &Cpu::BIT_r_m1<7, Register8::H> },
	/* 7D */ { &Cpu::BIT_r_m1<7, Register8::L> },
	/* 7E */ { &Cpu::BIT_arr_m1<7, Register16::HL>, &Cpu::BIT_arr_m2<7, Register16::HL> },
	/* 7F */ { &Cpu::BIT_r_m1<7, Register8::A> },
	/* 80 */ { &Cpu::RES_r_m1<0, Register8::B> },
	/* 81 */ { &Cpu::RES_r_m1<0, Register8::C> },
	/* 82 */ { &Cpu::RES_r_m1<0, Register8::D> },
	/* 83 */ { &Cpu::RES_r_m1<0, Register8::E> },
	/* 84 */ { &Cpu::RES_r_m1<0, Register8::H> },
	/* 85 */ { &Cpu::RES_r_m1<0, Register8::L> },
	/* 86 */ { &Cpu::RES_arr_m1<0, Register16::HL>, &Cpu::RES_arr_m2<0, Register16::HL>, &Cpu::RES_arr_m3<0, Register16::HL> },
	/* 87 */ { &Cpu::RES_r_m1<0, Register8::A> },
	/* 88 */ { &Cpu::RES_r_m1<1, Register8::B> },
	/* 89 */ { &Cpu::RES_r_m1<1, Register8::C> },
	/* 8A */ { &Cpu::RES_r_m1<1, Register8::D> },
	/* 8B */ { &Cpu::RES_r_m1<1, Register8::E> },
	/* 8C */ { &Cpu::RES_r_m1<1, Register8::H> },
	/* 8D */ { &Cpu::RES_r_m1<1, Register8::L> },
	/* 8E */ { &Cpu::RES_arr_m1<1, Register16::HL>, &Cpu::RES_arr_m2<1, Register16::HL>, &Cpu::RES_arr_m3<1, Register16::HL> },
	/* 8F */ { &Cpu::RES_r_m1<1, Register8::A> },
	/* 90 */ { &Cpu::RES_r_m1<2, Register8::B> },
	/* 91 */ { &Cpu::RES_r_m1<2, Register8::C> },
	/* 92 */ { &Cpu::RES_r_m1<2, Register8::D> },
	/* 93 */ { &Cpu::RES_r_m1<2, Register8::E> },
	/* 94 */ { &Cpu::RES_r_m1<2, Register8::H> },
	/* 95 */ { &Cpu::RES_r_m1<2, Register8::L> },
	/* 96 */ { &Cpu::RES_arr_m1<2, Register16::HL>, &Cpu::RES_arr_m2<2, Register16::HL>, &Cpu::RES_arr_m3<2, Register16::HL> },
	/* 97 */ { &Cpu::RES_r_m1<2, Register8::A> },
	/* 98 */ { &Cpu::RES_r_m1<3, Register8::B> },
	/* 99 */ { &Cpu::RES_r_m1<3, Register8::C> },
	/* 9A */ { &Cpu::RES_r_m1<3, Register8::D> },
	/* 9B */ { &Cpu::RES_r_m1<3, Register8::E> },
	/* 9C */ { &Cpu::RES_r_m1<3, Register8::H> },
	/* 9D */ { &Cpu::RES_r_m1<3, Register8::L> },
	/* 9E */ { &Cpu::RES_arr_m1<3, Register16::HL>, &Cpu::RES_arr_m2<3, Register16::HL>, &Cpu::RES_arr_m3<3, Register16::HL> },
	/* 9F */ { &Cpu::RES_r_m1<3, Register8::A> },
	/* A0 */ { &Cpu::RES_r_m1<4, Register8::B> },
	/* A1 */ { &Cpu::RES_r_m1<4, Register8::C> },
	/* A2 */ { &Cpu::RES_r_m1<4, Register8::D> },
	/* A3 */ { &Cpu::RES_r_m1<4, Register8::E> },
	/* A4 */ { &Cpu::RES_r_m1<4, Register8::H> },
	/* A5 */ { &Cpu::RES_r_m1<4, Register8::L> },
	/* A6 */ { &Cpu::RES_arr_m1<4, Register16::HL>, &Cpu::RES_arr_m2<4, Register16::HL>, &Cpu::RES_arr_m3<4, Register16::HL> },
	/* A7 */ { &Cpu::RES_r_m1<4, Register8::A> },
	/* A8 */ { &Cpu::RES_r_m1<5, Register8::B> },
	/* A9 */ { &Cpu::RES_r_m1<5, Register8::C> },
	/* AA */ { &Cpu::RES_r_m1<5, Register8::D> },
	/* AB */ { &Cpu::RES_r_m1<5, Register8::E> },
	/* AC */ { &Cpu::RES_r_m1<5, Register8::H> },
	/* AD */ { &Cpu::RES_r_m1<5, Register8::L> },
	/* AE */ { &Cpu::RES_arr_m1<5, Register16::HL>, &Cpu::RES_arr_m2<5, Register16::HL>, &Cpu::RES_arr_m3<5, Register16::HL> },
	/* AF */ { &Cpu::RES_r_m1<5, Register8::A> },
	/* B0 */ { &Cpu::RES_r_m1<6, Register8::B> },
	/* B1 */ { &Cpu::RES_r_m1<6, Register8::C> },
	/* B2 */ { &Cpu::RES_r_m1<6, Register8::D> },
	/* B3 */ { &Cpu::RES_r_m1<6, Register8::E> },
	/* B4 */ { &Cpu::RES_r_m1<6, Register8::H> },
	/* B5 */ { &Cpu::RES_r_m1<6, Register8::L> },
	/* B6 */ { &Cpu::RES_arr_m1<6, Register16::HL>, &Cpu::RES_arr_m2<6, Register16::HL>, &Cpu::RES_arr_m3<6, Register16::HL> },
	/* B7 */ { &Cpu::RES_r_m1<6, Register8::A> },
	/* B8 */ { &Cpu::RES_r_m1<7, Register8::B> },
	/* B9 */ { &Cpu::RES_r_m1<7, Register8::C> },
	/* BA */ { &Cpu::RES_r_m1<7, Register8::D> },
	/* BB */ { &Cpu::RES_r_m1<7, Register8::E> },
	/* BC */ { &Cpu::RES_r_m1<7, Register8::H> },
	/* BD */ { &Cpu::RES_r_m1<7, Register8::L> },
	/* BE */ { &Cpu::RES_arr_m1<7, Register16::HL>, &Cpu::RES_arr_m2<7, Register16::HL>, &Cpu::RES_arr_m3<7, Register16::HL> },
	/* BF */ { &Cpu::RES_r_m1<7, Register8::A> },
    /* C0 */ { &Cpu::SET_r_m1<0, Register8::B> },
	/* C1 */ { &Cpu::SET_r_m1<0, Register8::C> },
	/* C2 */ { &Cpu::SET_r_m1<0, Register8::D> },
	/* C3 */ { &Cpu::SET_r_m1<0, Register8::E> },
	/* C4 */ { &Cpu::SET_r_m1<0, Register8::H> },
	/* C5 */ { &Cpu::SET_r_m1<0, Register8::L> },
	/* C6 */ { &Cpu::SET_arr_m1<0, Register16::HL>, &Cpu::SET_arr_m2<0, Register16::HL>, &Cpu::SET_arr_m3<0, Register16::HL> },
	/* C7 */ { &Cpu::SET_r_m1<0, Register8::A> },
	/* C8 */ { &Cpu::SET_r_m1<1, Register8::B> },
	/* C9 */ { &Cpu::SET_r_m1<1, Register8::C> },
	/* CA */ { &Cpu::SET_r_m1<1, Register8::D> },
	/* CB */ { &Cpu::SET_r_m1<1, Register8::E> },
	/* CC */ { &Cpu::SET_r_m1<1, Register8::H> },
	/* CD */ { &Cpu::SET_r_m1<1, Register8::L> },
	/* CE */ { &Cpu::SET_arr_m1<1, Register16::HL>, &Cpu::SET_arr_m2<1, Register16::HL>, &Cpu::SET_arr_m3<1, Register16::HL> },
	/* CF */ { &Cpu::SET_r_m1<1, Register8::A> },
	/* D0 */ { &Cpu::SET_r_m1<2, Register8::B> },
	/* D1 */ { &Cpu::SET_r_m1<2, Register8::C> },
	/* D2 */ { &Cpu::SET_r_m1<2, Register8::D> },
	/* D3 */ { &Cpu::SET_r_m1<2, Register8::E> },
	/* D4 */ { &Cpu::SET_r_m1<2, Register8::H> },
	/* D5 */ { &Cpu::SET_r_m1<2, Register8::L> },
	/* D6 */ { &Cpu::SET_arr_m1<2, Register16::HL>, &Cpu::SET_arr_m2<2, Register16::HL>, &Cpu::SET_arr_m3<2, Register16::HL> },
	/* D7 */ { &Cpu::SET_r_m1<2, Register8::A> },
	/* D8 */ { &Cpu::SET_r_m1<3, Register8::B> },
	/* D9 */ { &Cpu::SET_r_m1<3, Register8::C> },
	/* DA */ { &Cpu::SET_r_m1<3, Register8::D> },
	/* DB */ { &Cpu::SET_r_m1<3, Register8::E> },
	/* DC */ { &Cpu::SET_r_m1<3, Register8::H> },
	/* DD */ { &Cpu::SET_r_m1<3, Register8::L> },
	/* DE */ { &Cpu::SET_arr_m1<3, Register16::HL>, &Cpu::SET_arr_m2<3, Register16::HL>, &Cpu::SET_arr_m3<3, Register16::HL> },
	/* DF */ { &Cpu::SET_r_m1<3, Register8::A> },
	/* E0 */ { &Cpu::SET_r_m1<4, Register8::B> },
	/* E1 */ { &Cpu::SET_r_m1<4, Register8::C> },
	/* E2 */ { &Cpu::SET_r_m1<4, Register8::D> },
	/* E3 */ { &Cpu::SET_r_m1<4, Register8::E> },
	/* E4 */ { &Cpu::SET_r_m1<4, Register8::H> },
	/* E5 */ { &Cpu::SET_r_m1<4, Register8::L> },
	/* E6 */ { &Cpu::SET_arr_m1<4, Register16::HL>, &Cpu::SET_arr_m2<4, Register16::HL>, &Cpu::SET_arr_m3<4, Register16::HL> },
	/* E7 */ { &Cpu::SET_r_m1<4, Register8::A> },
	/* E8 */ { &Cpu::SET_r_m1<5, Register8::B> },
	/* E9 */ { &Cpu::SET_r_m1<5, Register8::C> },
	/* EA */ { &Cpu::SET_r_m1<5, Register8::D> },
	/* EB */ { &Cpu::SET_r_m1<5, Register8::E> },
	/* EC */ { &Cpu::SET_r_m1<5, Register8::H> },
	/* ED */ { &Cpu::SET_r_m1<5, Register8::L> },
	/* EE */ { &Cpu::SET_arr_m1<5, Register16::HL>, &Cpu::SET_arr_m2<5, Register16::HL>, &Cpu::SET_arr_m3<5, Register16::HL> },
	/* EF */ { &Cpu::SET_r_m1<5, Register8::A> },
	/* F0 */ { &Cpu::SET_r_m1<6, Register8::B> },
	/* F1 */ { &Cpu::SET_r_m1<6, Register8::C> },
	/* F2 */ { &Cpu::SET_r_m1<6, Register8::D> },
	/* F3 */ { &Cpu::SET_r_m1<6, Register8::E> },
	/* F4 */ { &Cpu::SET_r_m1<6, Register8::H> },
	/* F5 */ { &Cpu::SET_r_m1<6, Register8::L> },
	/* F6 */ { &Cpu::SET_arr_m1<6, Register16::HL>, &Cpu::SET_arr_m2<6, Register16::HL>, &Cpu::SET_arr_m3<6, Register16::HL> },
	/* F7 */ { &Cpu::SET_r_m1<6, Register8::A> },
	/* F8 */ { &Cpu::SET_r_m1<7, Register8::B> },
	/* F9 */ { &Cpu::SET_r_m1<7, Register8::C> },
	/* FA */ { &Cpu::SET_r_m1<7, Register8::D> },
	/* FB */ { &Cpu::SET_r_m1<7, Register8::E> },
	/* FC */ { &Cpu::SET_r_m1<7, Register8::H> },
	/* FD */ { &Cpu::SET_r_m1<7, Register8::L> },
	/* FE */ { &Cpu::SET_arr_m1<7, Register16::HL>, &Cpu::SET_arr_m2<7, Register16::HL>, &Cpu::SET_arr_m3<7, Register16::HL> },
	/* FF */ { &Cpu::SET_r_m1<7, Register8::A> },
    }, ISR {
        /* VBLANK */ { &Cpu::ISR_m1<0x40>, &Cpu::ISR_m2<0x40>, &Cpu::ISR_m3<0x40>, &Cpu::ISR_m4<0x40>, &Cpu::ISR_m5<0x40> },
        /* STAT   */ { &Cpu::ISR_m1<0x48>, &Cpu::ISR_m2<0x48>, &Cpu::ISR_m3<0x48>, &Cpu::ISR_m4<0x48>, &Cpu::ISR_m5<0x48> },
        /* TIMER  */ { &Cpu::ISR_m1<0x50>, &Cpu::ISR_m2<0x50>, &Cpu::ISR_m3<0x50>, &Cpu::ISR_m4<0x50>, &Cpu::ISR_m5<0x50> },
        /* SERIAL */ { &Cpu::ISR_m1<0x58>, &Cpu::ISR_m2<0x58>, &Cpu::ISR_m3<0x58>, &Cpu::ISR_m4<0x58>, &Cpu::ISR_m5<0x58> },
        /* JOYPAD */ { &Cpu::ISR_m1<0x60>, &Cpu::ISR_m2<0x60>, &Cpu::ISR_m3<0x60>, &Cpu::ISR_m4<0x60>, &Cpu::ISR_m5<0x60> },

    } {
    reset();
}

void Cpu::reset() {
    mCycles = 0;
    AF = 0x01B0;
    BC = 0x0013;
    DE = 0x00D8;
    HL = 0x014D;
    PC = 0x0100;
    SP = 0xFFFE;
    IME = false;
    currentInstruction = CurrentInstruction();
    currentInstruction.microopHandler = instructions[0]; // NOP -> fetch
    b = false;
    u = 0;
    s = 0;
    uu = 0;
    lsb = 0;
    msb = 0;
    addr = 0;
}

void Cpu::tick() {
    if (halted) {
        uint8_t IE = bus.read(MemoryMap::IE);
        uint8_t IF = bus.read(MemoryMap::IO::IF);
        for (uint8_t b = 0; b <= 4; b++) {
            if (get_bit(IE, b) && get_bit(IF, b)) {
                halted = false;
                // TODO: figure out if the IF bit is set to 0
                //  and if so, whether it depends on the value of IME
                bus.write(MemoryMap::IO::IF, IF);
                if (IME) {
                    set_bit(IF, b, false);
                    pendingInterrupt = ISR[b];
                }
            }
        }
        return;
    }

    if (pendingInterrupt) {
        // oneliner for take the right address based on the ISR function pointer
        currentInstruction.ISR = true;
        currentInstruction.address = 0x40 + 8 * ((*pendingInterrupt - &ISR[0][0]) / 5);
        currentInstruction.microop = 0;
        currentInstruction.microopHandler = *pendingInterrupt;
        pendingInterrupt = std::nullopt;
         return; // TODO: return, else if or just if?
    }

    // TODO: else if or just if?
    else if (IME) {
        uint8_t IE = bus.read(MemoryMap::IE);
        uint8_t IF = bus.read(MemoryMap::IO::IF);
        for (uint8_t b = 0; b <= 4; b++) {
            if (get_bit(IE, b) && get_bit(IF, b)) {
                set_bit(IF, b, false);
                bus.write(MemoryMap::IO::IF, IF);
                IME = false;
                pendingInterrupt = ISR[b];
                break; // TODO: stop after first or handle them all?
            }
        }
    }

    InstructionMicroOperation micro_op = *currentInstruction.microopHandler;

    // Must be increased before execute micro op (so that fetch overwrites eventually)
    currentInstruction.microop++;
    currentInstruction.microopHandler++;

    (this->*micro_op)();

    mCycles++;
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::A>() const {
    return get_byte<1>(AF);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::B>() const {
    return get_byte<1>(BC);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::C>() const {
    return get_byte<0>(BC);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::D>() const {
    return get_byte<1>(DE);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::E>() const {
    return get_byte<0>(DE);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::F>() const {
    return get_byte<0>(AF) & 0xF0; // last four bits hardwired to 0
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::H>() const {
    return get_byte<1>(HL);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::L>() const {
    return get_byte<0>(HL);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::SP_S>() const {
    return get_byte<1>(SP);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::SP_P>() const {
    return get_byte<0>(SP);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::PC_P>() const {
    return get_byte<1>(PC);
}

template<>
uint8_t Cpu::readRegister8<Cpu::Register8::PC_C>() const {
    return get_byte<0>(PC);
}

template<>
void Cpu::writeRegister8<Cpu::Register8::A>(uint8_t value) {
    set_byte<1>(AF, value);
}

template<>
void Cpu::writeRegister8<Cpu::Register8::B>(uint8_t value) {
    set_byte<1>(BC, value);
}

template<>
void Cpu::writeRegister8<Cpu::Register8::C>(uint8_t value) {
    set_byte<0>(BC, value);
}

template<>
void Cpu::writeRegister8<Cpu::Register8::D>(uint8_t value) {
    set_byte<1>(DE, value);
}

template<>
void Cpu::writeRegister8<Cpu::Register8::E>(uint8_t value) {
    set_byte<0>(DE, value);
}

template<>
void Cpu::writeRegister8<Cpu::Register8::F>(uint8_t value) {
    set_byte<0>(AF, value & 0xF0);
}

template<>
void Cpu::writeRegister8<Cpu::Register8::H>(uint8_t value) {
    set_byte<1>(HL, value);
}

template<>
void Cpu::writeRegister8<Cpu::Register8::L>(uint8_t value) {
    set_byte<0>(HL, value);
}

template<>
uint16_t Cpu::readRegister16<Cpu::Register16::AF>() const {
    return AF & 0xFFF0;
}

template<>
uint16_t Cpu::readRegister16<Cpu::Register16::BC>() const {
    return BC;
}

template<>
uint16_t Cpu::readRegister16<Cpu::Register16::DE>() const {
    return DE;
}

template<>
uint16_t Cpu::readRegister16<Cpu::Register16::HL>() const {
    return HL;
}

template<>
uint16_t Cpu::readRegister16<Cpu::Register16::PC>() const {
    return PC;
}

template<>
uint16_t Cpu::readRegister16<Cpu::Register16::SP>() const {
    return SP;
}

template<>
void Cpu::writeRegister16<Cpu::Register16::AF>(uint16_t value) {
    AF = value & 0xFFF0;
}

template<>
void Cpu::writeRegister16<Cpu::Register16::BC>(uint16_t value) {
    BC = value;
}

template<>
void Cpu::writeRegister16<Cpu::Register16::DE>(uint16_t value) {
    DE = value;
}

template<>
void Cpu::writeRegister16<Cpu::Register16::HL>(uint16_t value) {
    HL = value;
}

template<>
void Cpu::writeRegister16<Cpu::Register16::PC>(uint16_t value) {
    PC = value;
}

template<>
void Cpu::writeRegister16<Cpu::Register16::SP>(uint16_t value) {
    SP = value;
}

template<>
[[nodiscard]] bool Cpu::readFlag<Cpu::Flag::Z>() const {
    return get_bit<Bits::Flags::Z>(AF);
}

template<>
[[nodiscard]] bool Cpu::readFlag<Cpu::Flag::N>() const {
    return get_bit<Bits::Flags::N>(AF);
}

template<>
[[nodiscard]] bool Cpu::readFlag<Cpu::Flag::H>() const {
    return get_bit<Bits::Flags::H>(AF);
}

template<>
[[nodiscard]] bool Cpu::readFlag<Cpu::Flag::C>() const {
    return get_bit<Bits::Flags::C>(AF);
}

template<Cpu::Flag f, bool y>
[[nodiscard]] bool Cpu::checkFlag() const {
    return readFlag<f>() == y;
}

template<>
void Cpu::writeFlag<Cpu::Flag::Z>(bool value) {
    set_bit<Bits::Flags::Z>(AF, value);
}

template<>
void Cpu::writeFlag<Cpu::Flag::N>(bool value) {
    set_bit<Bits::Flags::N>(AF, value);
}

template<>
void Cpu::writeFlag<Cpu::Flag::H>(bool value) {
    set_bit<Bits::Flags::H>(AF, value);
}

template<>
void Cpu::writeFlag<Cpu::Flag::C>(bool value) {
    set_bit<Bits::Flags::C>(AF, value);
}

// ============================= INSTRUCTIONS ==================================

void Cpu::fetch(bool cb) {
    DEBUG(2) << "<fetch>" << std::endl;
    currentInstruction.ISR = false;
    currentInstruction.address = PC;
    currentInstruction.microop = cb ? 1 : 0;
    if (cb)
        currentInstruction.microopHandler = instructions_cb[bus.read(PC++)];
    else
        currentInstruction.microopHandler = instructions[bus.read(PC++)];

}

void Cpu::invalidInstruction() {
    throw std::runtime_error("Invalid instruction at address "+ hex(currentInstruction.address) + ")");
}

template<uint16_t nn>
void Cpu::ISR_m1() {
}

template<uint16_t nn>
void Cpu::ISR_m2() {
}

template<uint16_t nn>
void Cpu::ISR_m3() {
    uu = PC - 1; // TODO: -1 because of prefetch but it's ugly
    bus.write(--SP, get_byte<1>(uu));
}

template<uint16_t nn>
void Cpu::ISR_m4() {
    bus.write(--SP, get_byte<0>(uu));
}

template<uint16_t nn>
void Cpu::ISR_m5() {
    PC = nn;
    fetch();
}


void Cpu::NOP_m1() {
    fetch();
}

void Cpu::STOP_m1() {
    // TODO: what to do here?
    fetch();
}

void Cpu::HALT_m1() {
    fetch();
    halted = true;
}

void Cpu::DI_m1() {
    IME = false;
    fetch();
}

void Cpu::EI_m1() {
    IME = true;
    fetch();
}


// e.g. 01 | LD BC,d16

template<Cpu::Register16 rr>
void Cpu::LD_rr_uu_m1() {
    lsb = bus.read(PC++);
}

template<Cpu::Register16 rr>
void Cpu::LD_rr_uu_m2() {
    msb = bus.read(PC++);
}

template<Cpu::Register16 rr>
void Cpu::LD_rr_uu_m3() {
    writeRegister16<rr>(concat_bytes(msb, lsb));
    fetch();
}

// e.g. 36 | LD (HL),d8

template<Cpu::Register16 rr>
void Cpu::LD_arr_u_m1() {
    u = bus.read(PC++);
}

template<Cpu::Register16 rr>
void Cpu::LD_arr_u_m2() {
    bus.write(readRegister16<rr>(), u);
}

template<Cpu::Register16 rr>
void Cpu::LD_arr_u_m3() {
    fetch();
}

// e.g. 02 | LD (BC),A

template<Cpu::Register16 rr, Cpu::Register8 r>
void Cpu::LD_arr_r_m1() {
    bus.write(readRegister16<rr>(), readRegister8<r>());
}

template<Cpu::Register16 rr, Cpu::Register8 r>
void Cpu::LD_arr_r_m2() {
    fetch();
}

// e.g. 22 | LD (HL+),A

template<Cpu::Register16 rr, Cpu::Register8 r, int8_t inc>
void Cpu::LD_arri_r_m1() {
    bus.write(readRegister16<rr>(), readRegister8<r>());
    writeRegister16<rr>(readRegister16<rr>() + inc);
}

template<Cpu::Register16 rr, Cpu::Register8 r, int8_t inc>
void Cpu::LD_arri_r_m2() {
    fetch();
}

// e.g. 06 | LD B,d8

template<Cpu::Register8 r>
void Cpu::LD_r_u_m1() {
    writeRegister8<r>(bus.read(PC++));
}

template<Cpu::Register8 r>
void Cpu::LD_r_u_m2() {
    fetch();
}

// e.g. 08 | LD (a16),SP

template<Cpu::Register16 rr>
void Cpu::LD_ann_rr_m1() {
    lsb = bus.read(PC++);
}

template<Cpu::Register16 rr>
void Cpu::LD_ann_rr_m2() {
    msb = bus.read(PC++);
}

template<Cpu::Register16 rr>
void Cpu::LD_ann_rr_m3() {
    addr = concat_bytes(msb, lsb);
    uu = readRegister16<rr>();
    bus.write(addr, get_byte<0>(uu));
    // TODO: P or S?
}

template<Cpu::Register16 rr>
void Cpu::LD_ann_rr_m4() {
    bus.write(addr + 1, get_byte<1>(uu));
    // TODO: P or S?
}

template<Cpu::Register16 rr>
void Cpu::LD_ann_rr_m5() {
    fetch();
}

// e.g. 0A |  LD A,(BC)

template<Cpu::Register8 r, Cpu::Register16 rr>
void Cpu::LD_r_arr_m1() {
    writeRegister8<r>(bus.read(readRegister16<rr>()));
}

template<Cpu::Register8 r, Cpu::Register16 rr>
void Cpu::LD_r_arr_m2() {
    fetch();
}

// e.g. 2A |  LD A,(HL+)

template<Cpu::Register8 r, Cpu::Register16 rr, int8_t inc>
void Cpu::LD_r_arri_m1() {
    writeRegister8<r>(bus.read(readRegister16<rr>()));
    writeRegister16<rr>(readRegister16<rr>() + inc);
}

template<Cpu::Register8 r, Cpu::Register16 rr, int8_t inc>
void Cpu::LD_r_arri_m2() {
    fetch();
}

// e.g. 41 |  LD B,C

template<Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LD_r_r_m1() {
    writeRegister8<r1>(readRegister8<r2>());
    fetch();
}

// e.g. E0 | LDH (a8),A

template<Cpu::Register8 r>
void Cpu::LDH_an_r_m1() {
    u = bus.read(PC++);
}

template<Cpu::Register8 r>
void Cpu::LDH_an_r_m2() {
    bus.write(concat_bytes(0xFF, u), readRegister8<r>());
}

template<Cpu::Register8 r>
void Cpu::LDH_an_r_m3() {
    fetch();
}

// e.g. F0 | LDH A,(a8)

template<Cpu::Register8 r>
void Cpu::LDH_r_an_m1() {
    u = bus.read(PC++);
}

template<Cpu::Register8 r>
void Cpu::LDH_r_an_m2() {
    writeRegister8<r>(bus.read(concat_bytes(0xFF, u)));
}

template<Cpu::Register8 r>
void Cpu::LDH_r_an_m3() {
    fetch();
}

// e.g. E2 | LD (C),A

template<Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LDH_ar_r_m1() {
    bus.write(concat_bytes(0xFF, readRegister8<r1>()), readRegister8<r2>());
}

template<Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LDH_ar_r_m2() {
    fetch();
}

// e.g. F2 | LD A,(C)

template<Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LDH_r_ar_m1() {
    writeRegister8<r1>(bus.read(concat_bytes(0xFF, readRegister8<r2>())));
}


template<Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::LDH_r_ar_m2() {
    fetch();
}

// e.g. EA | LD (a16),A

template<Cpu::Register8 r>
void Cpu::LD_ann_r_m1() {
    lsb = bus.read(PC++);
}

template<Cpu::Register8 r>
void Cpu::LD_ann_r_m2() {
    msb = bus.read(PC++);
}

template<Cpu::Register8 r>
void Cpu::LD_ann_r_m3() {
    bus.write(concat_bytes(msb, lsb), readRegister8<r>());
}

template<Cpu::Register8 r>
void Cpu::LD_ann_r_m4() {
    fetch();
}

// e.g. FA | LD A,(a16)

template<Cpu::Register8 r>
void Cpu::LD_r_ann_m1() {
    lsb = bus.read(PC++);
}

template<Cpu::Register8 r>
void Cpu::LD_r_ann_m2() {
    msb = bus.read(PC++);
}

template<Cpu::Register8 r>
void Cpu::LD_r_ann_m3() {
    writeRegister8<r>(bus.read(concat_bytes(msb, lsb)));
}

template<Cpu::Register8 r>
void Cpu::LD_r_ann_m4() {
    fetch();
}

// e.g. F9 | LD SP,HL

template<Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rr_m1() {
    writeRegister16<rr1>(readRegister16<rr2>());
    // TODO: i guess this should be split in the two 8bit registers between m1 and m2
}

template<Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rr_m2() {
    fetch();
}

// e.g. F8 | LD HL,SP+r8


template<Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rrs_m1() {
    s = static_cast<int8_t>(bus.read(PC++));
}

template<Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rrs_m2() {
    uu = readRegister16<rr2>();
    auto [result, h, c] = sum_carry<3, 7>(uu, s);
    writeRegister16<rr1>(result);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    // TODO: when are flags set?
    // TODO: does it work?
}

template<Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::LD_rr_rrs_m3() {
    fetch();
}

// e.g. 04 | INC B

template<Cpu::Register8 r>
void Cpu::INC_r_m1() {
    uint8_t tmp = readRegister8<r>();
    auto [result, h] = sum_carry<3>(tmp, 1);
    writeRegister8<r>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    fetch();
}


// e.g. 03 | INC BC

template<Cpu::Register16 rr>
void Cpu::INC_rr_m1() {
    uint16_t tmp = readRegister16<rr>();
    uint32_t result = tmp + 1;
    writeRegister16<rr>(result);
    // TODO: no flags?
}

template<Cpu::Register16 rr>
void Cpu::INC_rr_m2() {
    fetch();
}

// e.g. 34 | INC (HL)

template<Cpu::Register16 rr>
void Cpu::INC_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::INC_arr_m2() {
    auto [result, h] = sum_carry<3>(u, 1);
    bus.write(addr, result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
}

template<Cpu::Register16 rr>
void Cpu::INC_arr_m3() {
    fetch();
}

// e.g. 05 | DEC B

template<Cpu::Register8 r>
void Cpu::DEC_r_m1() {
    uint8_t tmp = readRegister8<r>();
    auto [result, h] = sub_borrow<3>(tmp, 1);
    writeRegister8<r>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    fetch();
}


// e.g. 0B | DEC BC

template<Cpu::Register16 rr>
void Cpu::DEC_rr_m1() {
    uint16_t tmp = readRegister16<rr>();
    uint32_t result = tmp - 1;
    writeRegister16<rr>(result);
    // TODO: no flags?
}

template<Cpu::Register16 rr>
void Cpu::DEC_rr_m2() {
    fetch();
}

// e.g. 35 | DEC (HL)

template<Cpu::Register16 rr>
void Cpu::DEC_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::DEC_arr_m2() {
    auto [result, h] = sub_borrow<3>(u, 1);
    bus.write(addr, result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
}

template<Cpu::Register16 rr>
void Cpu::DEC_arr_m3() {
    fetch();
}

// e.g. 80 | ADD B

template<Cpu::Register8 r>
void Cpu::ADD_r_m1() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 86 | ADD (HL)

template<Cpu::Register16 rr>
void Cpu::ADD_arr_m1() {
    u = bus.read(readRegister16<rr>());
}

template<Cpu::Register16 rr>
void Cpu::ADD_arr_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), u);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// C6 | ADD A,d8

void Cpu::ADD_u_m1() {
    u = bus.read(PC++);
}

void Cpu::ADD_u_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), u);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 88 | ADC B

template<Cpu::Register8 r>
void Cpu::ADC_r_m1() {
    // TODO: dont like this + C very much...
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    auto [result, h, c] = sum_carry<3, 7>(tmp, readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 8E | ADC (HL)

template<Cpu::Register16 rr>
void Cpu::ADC_arr_m1() {
    u = bus.read(readRegister16<rr>());
}

template<Cpu::Register16 rr>
void Cpu::ADC_arr_m2() {
    // TODO: is this ok?
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), u);
    auto [result, h, c] = sum_carry<3, 7>(tmp, readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// CE | ADC A,d8

void Cpu::ADC_u_m1() {
    u = bus.read(PC++);
}

void Cpu::ADC_u_m2() {
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), u);
    auto [result, h, c] = sum_carry<3, 7>(tmp, readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 09 | ADD HL,BC

template<Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ADD_rr_rr_m1() {
    auto [result, h, c] = sum_carry<11, 15>(readRegister16<rr1>(), readRegister16<rr2>());
    writeRegister16<rr1>(result);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
}

template<Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ADD_rr_rr_m2() {
    fetch();
}

// e.g. E8 | ADD SP,r8

template<Cpu::Register16 rr>
void Cpu::ADD_rr_s_m1() {
    s = static_cast<int8_t>(bus.read(PC++));
}

template<Cpu::Register16 rr>
void Cpu::ADD_rr_s_m2() {
    // TODO: is it ok to carry bit 3 and 7?
    auto [result, h, c] = sum_carry<3, 7>(readRegister16<rr>(), s);
    writeRegister16<rr>(result);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
}


template<Cpu::Register16 rr>
void Cpu::ADD_rr_s_m3() {
    // TODO: why? i guess something about the instruction timing is wrong
}


template<Cpu::Register16 rr>
void Cpu::ADD_rr_s_m4() {
    fetch();
}


// e.g. 90 | SUB B

template<Cpu::Register8 r>
void Cpu::SUB_r_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 96 | SUB (HL)

template<Cpu::Register16 rr>
void Cpu::SUB_arr_m1() {
    u = bus.read(readRegister16<rr>());
}

template<Cpu::Register16 rr>
void Cpu::SUB_arr_m2() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// D6 | SUB A,d8

void Cpu::SUB_u_m1() {
    u = bus.read(PC++);
}

void Cpu::SUB_u_m2() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 98 | SBC B

template<Cpu::Register8 r>
void Cpu::SBC_r_m1() {
    // TODO: is this ok?
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 9E | SBC A,(HL)

template<Cpu::Register16 rr>
void Cpu::SBC_arr_m1() {
    u = bus.read(readRegister16<rr>());
}

template<Cpu::Register16 rr>
void Cpu::SBC_arr_m2() {
    // TODO: dont like this - C very much...
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}


// D6 | SBC A,d8

void Cpu::SBC_u_m1() {
    u = bus.read(PC++);
}

void Cpu::SBC_u_m2() {
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. A0 | AND B

template<Cpu::Register8 r>
void Cpu::AND_r_m1() {
    uint8_t result = readRegister8<Register8::A>() & readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}

// e.g. A6 | AND (HL)

template<Cpu::Register16 rr>
void Cpu::AND_arr_m1() {
    u = bus.read(readRegister16<rr>());
}

template<Cpu::Register16 rr>
void Cpu::AND_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() & u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}

// E6 | AND d8

void Cpu::AND_u_m1() {
    u = bus.read(PC++);
}

void Cpu::AND_u_m2() {
    uint8_t result = readRegister8<Register8::A>() & u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}


// B0 | OR B

template<Cpu::Register8 r>
void Cpu::OR_r_m1() {
    uint8_t result = readRegister8<Register8::A>() | readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

// e.g. B6 | OR (HL)

template<Cpu::Register16 rr>
void Cpu::OR_arr_m1() {
    u = bus.read(readRegister16<rr>());
}

template<Cpu::Register16 rr>
void Cpu::OR_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() | u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


// F6 | OR d8

void Cpu::OR_u_m1() {
    u = bus.read(PC++);
}

void Cpu::OR_u_m2() {
    uint8_t result = readRegister8<Register8::A>() | u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


// e.g. A8 | XOR B

template<Cpu::Register8 r>
void Cpu::XOR_r_m1() {
    uint8_t result = readRegister8<Register8::A>() ^ readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

template<Cpu::Register16 rr>
void Cpu::XOR_arr_m1() {
    u = bus.read(readRegister16<rr>());
}

template<Cpu::Register16 rr>
void Cpu::XOR_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() ^ u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

// EE | XOR d8

void Cpu::XOR_u_m1() {
    u = bus.read(PC++);
}

void Cpu::XOR_u_m2() {
    uint8_t result = readRegister8<Register8::A>() ^ u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


template<Cpu::Register8 r>
void Cpu::CP_r_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

template<Cpu::Register16 rr>
void Cpu::CP_arr_m1() {
    u = bus.read(readRegister16<rr>());
}

template<Cpu::Register16 rr>
void Cpu::CP_arr_m2() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// FE | CP d8

void Cpu::CP_u_m1() {
    u = bus.read(PC++);
}

void Cpu::CP_u_m2() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// 27 | DAA

void Cpu::DAA_m1() {
    u = readRegister8<Register8::A>();
    auto N = readFlag<Flag::N>();
    auto H = readFlag<Flag::H>();
    auto C = readFlag<Flag::C>();

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
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(C);
    fetch();
}

// 37 | SCF

void Cpu::SCF_m1() {
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(true);
    fetch();
}

// 2F | CPL

void Cpu::CPL_m1() {
    writeRegister8<Register8::A>(~readRegister8<Register8::A>());
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(true);
    fetch();
}

// 3F | CCF_m1

void Cpu::CCF_m1() {
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(!readFlag<Flag::C>());
    fetch();
}

// 07 | RLCA

void Cpu::RLCA_m1() {
    u = readRegister8<Register8::A>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | b7;
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// 17 | RLA

void Cpu::RLA_m1() {
    u = readRegister8<Register8::A>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | readFlag<Flag::C>();
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// 0F | RRCA

void Cpu::RRCA_m1() {
    u = readRegister8<Register8::A>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// 1F | RRA

void Cpu::RRA_m1() {
    u = readRegister8<Register8::A>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (readFlag<Flag::C>() << 7);
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. 18 | JR r8

void  Cpu::JR_s_m1() {
    s = static_cast<int8_t>(bus.read(PC++));
}

void  Cpu::JR_s_m2() {
    PC = (int16_t) PC + s;
}

void  Cpu::JR_s_m3() {
    fetch();
}

// e.g. 28 | JR Z,r8
// e.g. 20 | JR NZ,r8

template<Cpu::Flag f, bool y>
void Cpu::JR_c_s_m1() {
    s = static_cast<int8_t>(bus.read(PC++));
}

template<Cpu::Flag f, bool y>
void Cpu::JR_c_s_m2() {
    if (checkFlag<f, y>()) {
        PC = PC + s;
    } else {
        fetch();
    }
}

template<Cpu::Flag f, bool y>
void Cpu::JR_c_s_m3() {
    fetch();
}



// e.g. C3 | JP a16

void  Cpu::JP_uu_m1() {
    lsb = bus.read(PC++);
}

void Cpu::JP_uu_m2() {
    msb = bus.read(PC++);
}

void Cpu::JP_uu_m3() {
    PC = concat_bytes(msb, lsb);
}

void Cpu::JP_uu_m4() {
    fetch();
}

// e.g. E9 | JP (HL)

template<Cpu::Register16 rr>
void Cpu::JP_rr_m1() {
    PC = readRegister16<rr>();
    fetch();
}

// e.g. CA | JP Z,a16
// e.g. C2 | JP NZ,a16

template<Cpu::Flag f, bool y>
void Cpu::JP_c_uu_m1() {
    lsb = bus.read(PC++);
}

template<Cpu::Flag f, bool y>
void Cpu::JP_c_uu_m2() {
    msb = bus.read(PC++);
}

template<Cpu::Flag f, bool y>
void Cpu::JP_c_uu_m3() {
    if (checkFlag<f, y>()) {
        PC = concat_bytes(msb, lsb);
    } else {
        fetch();
    }
}

template<Cpu::Flag f, bool y>
void Cpu::JP_c_uu_m4() {
    fetch();
}

// CD | CALL a16

void Cpu::CALL_uu_m1() {
    lsb = bus.read(PC++);
}

void Cpu::CALL_uu_m2() {
    msb = bus.read(PC++);
}

void Cpu::CALL_uu_m3() {
    bus.write(--SP, readRegister8<Register8::PC_P>());
}

void Cpu::CALL_uu_m4() {
    bus.write(--SP, readRegister8<Register8::PC_C>());
}

void Cpu::CALL_uu_m5() {
    PC = concat_bytes(msb, lsb);
}

void Cpu::CALL_uu_m6() {
    fetch();
}

// e.g. C4 | CALL NZ,a16

template<Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m1() {
    lsb = bus.read(PC++);
}

template<Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m2() {
    msb = bus.read(PC++);
}
template<Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m3() {
    if (checkFlag<f, y>()) {
        bus.write(--SP, readRegister8<Register8::PC_P>());
    } else {
        fetch();
    }
}

template<Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m4() {
    bus.write(--SP, readRegister8<Register8::PC_C>());
}

template<Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m5() {
    PC = concat_bytes(msb, lsb);
}

template<Cpu::Flag f, bool y>
void Cpu::CALL_c_uu_m6() {
    fetch();
}

// C7 | RST 00H

template<uint8_t n>
void Cpu::RST_m1() {
    bus.write(--SP, readRegister8<Register8::PC_P>());
}

template<uint8_t n>
void Cpu::RST_m2() {
    bus.write(--SP, readRegister8<Register8::PC_C>());
}

template<uint8_t n>
void Cpu::RST_m3() {
    PC = concat_bytes(0x00, n);
}

template<uint8_t n>
void Cpu::RST_m4() {
    fetch();
}

// C9 | RET

void Cpu::RET_uu_m1() {
    lsb = bus.read(SP++);
}

void Cpu::RET_uu_m2() {
    msb = bus.read(SP++);
}

void Cpu::RET_uu_m3() {
    PC = concat_bytes(msb, lsb);
}

void Cpu::RET_uu_m4() {
    fetch();
}

// D9 | RETI

void Cpu::RETI_uu_m1() {
    lsb = bus.read(SP++);
}

void Cpu::RETI_uu_m2() {
    msb = bus.read(SP++);
}

void Cpu::RETI_uu_m3() {
    PC = concat_bytes(msb, lsb);
    IME = true;
}

void Cpu::RETI_uu_m4() {
    fetch();
}

// e.g. C0 | RET NZ

template<Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m1() {
    // TODO: really bad but don't know why this lasts 2 m cycle if false
}

template<Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m2() {
    // TODO: really bad but don't know why this lasts 2 m cycle if false
    if (checkFlag<f, y>()) {
        lsb = bus.read(SP++);
    } else {
        fetch();
    }
}

template<Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m3() {
    msb = bus.read(SP++);
}

template<Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m4() {
    PC = concat_bytes(msb, lsb);
}

template<Cpu::Flag f, bool y>
void Cpu::RET_c_uu_m5() {
    fetch();
}

// e.g. C5 | PUSH BC

template<Cpu::Register16 rr>
void Cpu::PUSH_rr_m1() {
    uu = readRegister16<rr>();
}

template<Cpu::Register16 rr>
void Cpu::PUSH_rr_m2() {
    bus.write(--SP, get_byte<1>(uu));
}

template<Cpu::Register16 rr>
void Cpu::PUSH_rr_m3() {
    bus.write(--SP, get_byte<0>(uu));
}

template<Cpu::Register16 rr>
void Cpu::PUSH_rr_m4() {
    fetch();
}

// e.g. C1 | POP BC

template<Cpu::Register16 rr>
void Cpu::POP_rr_m1() {
    lsb = bus.read(SP++);
}

template<Cpu::Register16 rr>
void Cpu::POP_rr_m2() {
    msb = bus.read(SP++);
}

template<Cpu::Register16 rr>
void Cpu::POP_rr_m3() {
    writeRegister16<rr>(concat_bytes(msb, lsb));
    fetch();
}

void Cpu::CB_m1() {
    fetch(true);
}

// e.g. e.g. CB 00 | RLC B

template<Cpu::Register8 r>
void Cpu::RLC_r_m1() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | b7;
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// e.g. e.g. CB 06 | RLC (HL)

template<Cpu::Register16 rr>
void Cpu::RLC_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::RLC_arr_m2() {
    bool b7 = get_bit<7>(u);
    u = (u << 1) | b7;
    bus.write(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
}

template<Cpu::Register16 rr>
void Cpu::RLC_arr_m3() {
    fetch();
}

// e.g. e.g. CB 08 | RRC B

template<Cpu::Register8 r>
void Cpu::RRC_r_m1() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. e.g. CB 0E | RRC (HL)

template<Cpu::Register16 rr>
void Cpu::RRC_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::RRC_arr_m2() {
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    bus.write(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
}

template<Cpu::Register16 rr>
void Cpu::RRC_arr_m3() {
    fetch();
}


// e.g. e.g. CB 10 | RL B

template<Cpu::Register8 r>
void Cpu::RL_r_m1() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | readFlag<Flag::C>();
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// e.g. e.g. CB 16 | RL (HL)

template<Cpu::Register16 rr>
void Cpu::RL_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::RL_arr_m2() {
    bool b7 = get_bit<7>(u);
    u = (u << 1) | readFlag<Flag::C>();
    bus.write(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
}

template<Cpu::Register16 rr>
void Cpu::RL_arr_m3() {
    fetch();
}

// e.g. e.g. CB 18 | RR B

template<Cpu::Register8 r>
void Cpu::RR_r_m1() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (readFlag<Flag::C>() << 7);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. e.g. CB 1E | RR (HL)

template<Cpu::Register16 rr>
void Cpu::RR_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::RR_arr_m2() {
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (readFlag<Flag::C>() << 7);
    bus.write(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
}

template<Cpu::Register16 rr>
void Cpu::RR_arr_m3() {
    fetch();
}


// e.g. CB 28 | SRA B

template<Cpu::Register8 r>
void Cpu::SRA_r_m1() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (u & bit<7>);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. CB 2E | SRA (HL)

template<Cpu::Register16 rr>
void Cpu::SRA_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::SRA_arr_m2() {
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (u & bit<7>);
    bus.write(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
}

template<Cpu::Register16 rr>
void Cpu::SRA_arr_m3() {
    fetch();
}


// e.g. CB 38 | SRL B

template<Cpu::Register8 r>
void Cpu::SRL_r_m1() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. CB 3E | SRL (HL)

template<Cpu::Register16 rr>
void Cpu::SRL_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::SRL_arr_m2() {
    bool b0 = get_bit<0>(u);
    u = (u >> 1);
    bus.write(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
}


template<Cpu::Register16 rr>
void Cpu::SRL_arr_m3() {
    fetch();
}

// e.g. CB 20 | SLA B

template<Cpu::Register8 r>
void Cpu::SLA_r_m1() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = u << 1;
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// e.g. CB 26 | SLA (HL)

template<Cpu::Register16 rr>
void Cpu::SLA_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::SLA_arr_m2() {
    bool b7 = get_bit<7>(u);
    u = u << 1;
    bus.write(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
}

template<Cpu::Register16 rr>
void Cpu::SLA_arr_m3() {
    fetch();
}

// e.g. CB 30 | SWAP B

template<Cpu::Register8 r>
void Cpu::SWAP_r_m1() {
    u = readRegister8<r>();
    u = ((u & 0x0F) << 4) | ((u & 0xF0) >> 4);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

// e.g. CB 36 | SWAP (HL)

template<Cpu::Register16 rr>
void Cpu::SWAP_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<Cpu::Register16 rr>
void Cpu::SWAP_arr_m2() {
    u = ((u & 0x0F) << 4) | ((u & 0xF0) >> 4);
    bus.write(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
}

template<Cpu::Register16 rr>
void Cpu::SWAP_arr_m3() {
    fetch();
}

// e.g. CB 40 | BIT 0,B

template<uint8_t n, Cpu::Register8 r>
void Cpu::BIT_r_m1() {
    b = get_bit<n>(readRegister8<r>());
    writeFlag<Flag::Z>(b == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    fetch();
}

// e.g. CB 46 | BIT 0,(HL)

template<uint8_t n, Cpu::Register16 rr>
void Cpu::BIT_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<uint8_t n, Cpu::Register16 rr>
void Cpu::BIT_arr_m2() {
    b = get_bit<n>(u);
    writeFlag<Flag::Z>(b == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    fetch();
}

// e.g. CB 80 | RES 0,B

template<uint8_t n, Cpu::Register8 r>
void Cpu::RES_r_m1() {
    u = readRegister8<r>();
    set_bit<n>(u, false);
    writeRegister8<r>(u);
    fetch();
}

// e.g. CB 86 | RES 0,(HL)

template<uint8_t n, Cpu::Register16 rr>
void Cpu::RES_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<uint8_t n, Cpu::Register16 rr>
void Cpu::RES_arr_m2() {
    set_bit<n>(u, false);
    bus.write(addr, u);
}

template<uint8_t n, Cpu::Register16 rr>
void Cpu::RES_arr_m3() {
    fetch();
}


// e.g. CB C0 | SET 0,B

template<uint8_t n, Cpu::Register8 r>
void Cpu::SET_r_m1() {
    u = readRegister8<r>();
    set_bit<n>(u, true);
    writeRegister8<r>(u);
    fetch();
}

// e.g. CB C6 | SET 0,(HL)

template<uint8_t n, Cpu::Register16 rr>
void Cpu::SET_arr_m1() {
    addr = readRegister16<rr>();
    u = bus.read(addr);
}

template<uint8_t n, Cpu::Register16 rr>
void Cpu::SET_arr_m2() {
    set_bit<n>(u, true);
    bus.write(addr, u);
}

template<uint8_t n, Cpu::Register16 rr>
void Cpu::SET_arr_m3() {
    fetch();
}