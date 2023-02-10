#include "cpu.h"
#include "log/log.h"
#include "utils/binutils.h"
#include <functional>
#include <cassert>

CPU::InstructionNotImplementedException::InstructionNotImplementedException(const std::string &what) : logic_error(what) {

}
CPU::IllegalInstructionException::IllegalInstructionException(const std::string &what) : logic_error(what) {

}

CPU::CPU(IBus &bus) :
        bus(bus),
        mCycles(0),
        PC(0x100),
        SP(0),
        AF(0),
        BC(0),
        DE(0),
        HL(0),
        IME(false),
        currentInstruction(),
        u1(),
        u2(),
        uu1(),
        instructions {

	/* 00 */ { &CPU::NOP_m1 },
	/* 01 */ { &CPU::LD_rr_nn_m1<Register16::BC>, &CPU::LD_rr_nn_m2<Register16::BC>, &CPU::LD_rr_nn_m3<Register16::BC> },
	/* 02 */ { &CPU::LD_arr_r_m1<Register16::BC, Register8::A>, &CPU::LD_arr_r_m2<Register16::BC, Register8::A> },
	/* 03 */ { &CPU::INC_rr_m1<Register16::BC>, &CPU::INC_rr_m2<Register16::BC> },
	/* 04 */ { &CPU::INC_r_m1<Register8::B> },
	/* 05 */ { &CPU::DEC_r_m1<Register8::B> },
	/* 06 */ { &CPU::LD_r_n_m1<Register8::B>, &CPU::LD_r_n_m2<Register8::B> },
	/* 07 */ { &CPU::RLCA_m1 },
	/* 08 */ { &CPU::LD_ann_rr_m1<Register16::SP>, &CPU::LD_ann_rr_m2<Register16::SP>, &CPU::LD_ann_rr_m3<Register16::SP>, &CPU::LD_ann_rr_m4<Register16::SP>, &CPU::LD_ann_rr_m5<Register16::SP> },
	/* 09 */ { &CPU::ADD_rr_rr_m1<Register16::HL, Register16::BC>, &CPU::ADD_rr_rr_m2<Register16::HL, Register16::BC> },
	/* 0A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::BC>, &CPU::LD_r_arr_m2<Register8::A, Register16::BC> },
	/* 0B */ { &CPU::DEC_rr_m1<Register16::BC>, &CPU::DEC_rr_m2<Register16::BC> },
	/* 0C */ { &CPU::INC_r_m1<Register8::C> },
	/* 0D */ { &CPU::DEC_r_m1<Register8::D> },
	/* 0E */ { &CPU::LD_r_n_m1<Register8::C>, &CPU::LD_r_n_m2<Register8::C> },
	/* 0F */ { &CPU::RRCA_m1 },
	/* 10 */ { &CPU::STOP_m1 },
	/* 11 */ { &CPU::LD_rr_nn_m1<Register16::DE>, &CPU::LD_rr_nn_m2<Register16::DE>, &CPU::LD_rr_nn_m3<Register16::DE> },
	/* 12 */ { &CPU::LD_arr_r_m1<Register16::DE, Register8::A>, &CPU::LD_arr_r_m2<Register16::DE, Register8::A> },
	/* 13 */ { &CPU::INC_rr_m1<Register16::DE>, &CPU::INC_rr_m2<Register16::DE> },
	/* 14 */ { &CPU::INC_r_m1<Register8::D> },
	/* 15 */ { &CPU::DEC_r_m1<Register8::D> },
	/* 16 */ { &CPU::LD_r_n_m1<Register8::D>, &CPU::LD_r_n_m2<Register8::D> },
	/* 17 */ { &CPU::RLA_m1 },
	/* 18 */ { &CPU::JR_n_m1, &CPU::JR_n_m2, &CPU::JR_n_m3 },
	/* 19 */ { &CPU::ADD_rr_rr_m1<Register16::HL, Register16::DE>, &CPU::ADD_rr_rr_m2<Register16::HL, Register16::DE> },
	/* 1A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::DE>, &CPU::LD_r_arr_m2<Register8::A, Register16::DE> },
	/* 1B */ { &CPU::DEC_rr_m1<Register16::DE>, &CPU::DEC_rr_m2<Register16::DE> },
	/* 1C */ { &CPU::INC_r_m1<Register8::E> },
	/* 1D */ { &CPU::DEC_r_m1<Register8::E> },
	/* 1E */ { &CPU::LD_r_n_m1<Register8::E>, &CPU::LD_r_n_m2<Register8::E> },
	/* 1F */ { &CPU::RRA_m1 },
	/* 20 */ { &CPU::JR_cc_n_m1<Flag::N, Flag::Z>, &CPU::JR_cc_n_m2<Flag::N, Flag::Z>, &CPU::JR_cc_n_m3<Flag::N, Flag::Z> },
	/* 21 */ { &CPU::LD_rr_nn_m1<Register16::HL>, &CPU::LD_rr_nn_m2<Register16::HL>, &CPU::LD_rr_nn_m3<Register16::HL> },
	/* 22 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::A, 1>, &CPU::LD_arr_r_m2<Register16::HL, Register8::A, 1> },
	/* 23 */ { &CPU::INC_rr_m1<Register16::HL>, &CPU::INC_rr_m2<Register16::HL> },
	/* 24 */ { &CPU::INC_r_m1<Register8::H> },
	/* 25 */ { &CPU::DEC_r_m1<Register8::H> },
	/* 26 */ { &CPU::LD_r_n_m1<Register8::H>, &CPU::LD_r_n_m2<Register8::H> },
	/* 27 */ { &CPU::DAA_m1 },
	/* 28 */ { &CPU::JR_c_n_m1<Flag::Z>, &CPU::JR_c_n_m2<Flag::Z>, &CPU::JR_c_n_m3<Flag::Z> },
	/* 29 */ { &CPU::ADD_rr_rr_m1<Register16::HL, Register16::HL>, &CPU::ADD_rr_rr_m2<Register16::HL, Register16::HL> },
	/* 2A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::HL, 1>, &CPU::LD_r_arr_m2<Register8::A, Register16::HL, 1> },
	/* 2B */ { &CPU::DEC_rr_m1<Register16::HL>, &CPU::DEC_rr_m2<Register16::HL> },
	/* 2C */ { &CPU::INC_r_m1<Register8::L> },
	/* 2D */ { &CPU::DEC_r_m1<Register8::L> },
	/* 2E */ { &CPU::LD_r_n_m1<Register8::L>, &CPU::LD_r_n_m2<Register8::L> },
	/* 2F */ { &CPU::CPL_m1 },
	/* 30 */ { &CPU::JR_cc_n_m1<Flag::N, Flag::C>, &CPU::JR_cc_n_m2<Flag::N, Flag::C>, &CPU::JR_cc_n_m3<Flag::N, Flag::C> },
	/* 31 */ { &CPU::LD_rr_nn_m1<Register16::SP>, &CPU::LD_rr_nn_m2<Register16::SP>, &CPU::LD_rr_nn_m3<Register16::SP> },
	/* 32 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::A, -1>, &CPU::LD_arr_r_m2<Register16::HL, Register8::A, -1> },
	/* 33 */ { &CPU::INC_rr_m1<Register16::SP>, &CPU::INC_rr_m2<Register16::SP> },
	/* 34 */ { &CPU::INC_arr_m1<Register16::HL>, &CPU::INC_arr_m2<Register16::HL>, &CPU::INC_arr_m3<Register16::HL> },
	/* 35 */ { &CPU::DEC_arr_m1<Register16::HL>, &CPU::DEC_arr_m2<Register16::HL>, &CPU::DEC_arr_m3<Register16::HL> },
	/* 36 */ { &CPU::LD_arr_n_m1<Register16::HL>, &CPU::LD_arr_n_m2<Register16::HL>, &CPU::LD_arr_n_m3<Register16::HL> },
	/* 37 */ { &CPU::SCF_m1 },
	/* 38 */ { &CPU::JR_c_n_m1<Flag::C>, &CPU::JR_c_n_m2<Flag::C>, &CPU::JR_c_n_m3<Flag::C> },
	/* 39 */ { &CPU::ADD_rr_rr_m1<Register16::HL, Register16::SP>, &CPU::ADD_rr_rr_m2<Register16::HL, Register16::SP> },
	/* 3A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::HL, -1>, &CPU::LD_r_arr_m2<Register8::A, Register16::HL, -1> },
	/* 3B */ { &CPU::DEC_rr_m1<Register16::SP>, &CPU::DEC_rr_m2<Register16::SP> },
	/* 3C */ { &CPU::INC_r_m1<Register8::C> },
	/* 3D */ { &CPU::DEC_r_m1<Register8::A> },
	/* 3E */ { &CPU::LD_r_n_m1<Register8::A>, &CPU::LD_r_n_m2<Register8::A> },
	/* 3F */ { &CPU::CCF_m1 },
	/* 40 */ { &CPU::LD_r_r_m1<Register8::B, Register8::B> },
	/* 41 */ { &CPU::LD_r_r_m1<Register8::B, Register8::C> },
	/* 42 */ { &CPU::LD_r_r_m1<Register8::B, Register8::D> },
	/* 43 */ { &CPU::LD_r_r_m1<Register8::B, Register8::E> },
	/* 44 */ { &CPU::LD_r_r_m1<Register8::B, Register8::H> },
	/* 45 */ { &CPU::LD_r_r_m1<Register8::B, Register8::L> },
	/* 46 */ { &CPU::LD_r_arr_m1<Register8::B, Register16::HL>, &CPU::LD_r_arr_m2<Register8::B, Register16::HL> },
	/* 47 */ { &CPU::LD_r_r_m1<Register8::B, Register8::A> },
	/* 48 */ { &CPU::LD_r_r_m1<Register8::C, Register8::B> },
	/* 49 */ { &CPU::LD_r_r_m1<Register8::C, Register8::C> },
	/* 4A */ { &CPU::LD_r_r_m1<Register8::C, Register8::D> },
	/* 4B */ { &CPU::LD_r_r_m1<Register8::C, Register8::E> },
	/* 4C */ { &CPU::LD_r_r_m1<Register8::C, Register8::H> },
	/* 4D */ { &CPU::LD_r_r_m1<Register8::C, Register8::L> },
	/* 4E */ { &CPU::LD_r_arr_m1<Register8::C, Register16::HL>, &CPU::LD_r_arr_m2<Register8::C, Register16::HL> },
	/* 4F */ { &CPU::LD_r_r_m1<Register8::A, Register8::A> },
	/* 50 */ { &CPU::LD_r_r_m1<Register8::D, Register8::B> },
	/* 51 */ { &CPU::LD_r_r_m1<Register8::D, Register8::C> },
	/* 52 */ { &CPU::LD_r_r_m1<Register8::D, Register8::D> },
	/* 53 */ { &CPU::LD_r_r_m1<Register8::D, Register8::E> },
	/* 54 */ { &CPU::LD_r_r_m1<Register8::D, Register8::H> },
	/* 55 */ { &CPU::LD_r_r_m1<Register8::D, Register8::L> },
	/* 56 */ { &CPU::LD_r_arr_m1<Register8::D, Register16::HL>, &CPU::LD_r_arr_m2<Register8::D, Register16::HL> },
	/* 57 */ { &CPU::LD_r_r_m1<Register8::D, Register8::A> },
	/* 58 */ { &CPU::LD_r_r_m1<Register8::E, Register8::B> },
	/* 59 */ { &CPU::LD_r_r_m1<Register8::E, Register8::C> },
	/* 5A */ { &CPU::LD_r_r_m1<Register8::E, Register8::D> },
	/* 5B */ { &CPU::LD_r_r_m1<Register8::E, Register8::E> },
	/* 5C */ { &CPU::LD_r_r_m1<Register8::E, Register8::H> },
	/* 5D */ { &CPU::LD_r_r_m1<Register8::E, Register8::L> },
	/* 5E */ { &CPU::LD_r_arr_m1<Register8::E, Register16::HL>, &CPU::LD_r_arr_m2<Register8::E, Register16::HL> },
	/* 5F */ { &CPU::LD_r_r_m1<Register8::E, Register8::A> },
	/* 60 */ { &CPU::LD_r_r_m1<Register8::H, Register8::B> },
	/* 61 */ { &CPU::LD_r_r_m1<Register8::H, Register8::C> },
	/* 62 */ { &CPU::LD_r_r_m1<Register8::H, Register8::D> },
	/* 63 */ { &CPU::LD_r_r_m1<Register8::H, Register8::E> },
	/* 64 */ { &CPU::LD_r_r_m1<Register8::H, Register8::H> },
	/* 65 */ { &CPU::LD_r_r_m1<Register8::H, Register8::L> },
	/* 66 */ { &CPU::LD_r_arr_m1<Register8::H, Register16::HL>, &CPU::LD_r_arr_m2<Register8::H, Register16::HL> },
	/* 67 */ { &CPU::LD_r_r_m1<Register8::H, Register8::A> },
	/* 68 */ { &CPU::LD_r_r_m1<Register8::L, Register8::B> },
	/* 69 */ { &CPU::LD_r_r_m1<Register8::L, Register8::C> },
	/* 6A */ { &CPU::LD_r_r_m1<Register8::L, Register8::D> },
	/* 6B */ { &CPU::LD_r_r_m1<Register8::L, Register8::E> },
	/* 6C */ { &CPU::LD_r_r_m1<Register8::L, Register8::H> },
	/* 6D */ { &CPU::LD_r_r_m1<Register8::L, Register8::L> },
	/* 6E */ { &CPU::LD_r_arr_m1<Register8::L, Register16::HL>, &CPU::LD_r_arr_m2<Register8::L, Register16::HL> },
	/* 6F */ { &CPU::LD_r_r_m1<Register8::L, Register8::A> },
	/* 70 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::B>, &CPU::LD_arr_r_m2<Register16::HL, Register8::B> },
	/* 71 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::C>, &CPU::LD_arr_r_m2<Register16::HL, Register8::C> },
	/* 72 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::D>, &CPU::LD_arr_r_m2<Register16::HL, Register8::D> },
	/* 73 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::E>, &CPU::LD_arr_r_m2<Register16::HL, Register8::E> },
	/* 74 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::H>, &CPU::LD_arr_r_m2<Register16::HL, Register8::H> },
	/* 75 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::L>, &CPU::LD_arr_r_m2<Register16::HL, Register8::L> },
	/* 76 */ { &CPU::HALT_m1 },
	/* 77 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::A>, &CPU::LD_arr_r_m2<Register16::HL, Register8::A> },
	/* 78 */ { &CPU::LD_r_r_m1<Register8::A, Register8::B> },
	/* 79 */ { &CPU::LD_r_r_m1<Register8::A, Register8::C> },
	/* 7A */ { &CPU::LD_r_r_m1<Register8::A, Register8::D> },
	/* 7B */ { &CPU::LD_r_r_m1<Register8::A, Register8::E> },
	/* 7C */ { &CPU::LD_r_r_m1<Register8::A, Register8::H> },
	/* 7D */ { &CPU::LD_r_r_m1<Register8::A, Register8::L> },
	/* 7E */ { &CPU::LD_r_arr_m1<Register8::A, Register16::HL>, &CPU::LD_r_arr_m2<Register8::A, Register16::HL> },
	/* 7F */ { &CPU::LD_r_r_m1<Register8::A, Register8::A> },
	/* 80 */ { &CPU::ADD_r_m1<Register8::B> },
	/* 81 */ { &CPU::ADD_r_m1<Register8::C> },
	/* 82 */ { &CPU::ADD_r_m1<Register8::D> },
	/* 83 */ { &CPU::ADD_r_m1<Register8::E> },
	/* 84 */ { &CPU::ADD_r_m1<Register8::H> },
	/* 85 */ { &CPU::ADD_r_m1<Register8::L> },
	/* 86 */ { &CPU::ADD_arr_m1<Register16::HL>, &CPU::ADD_arr_m2<Register16::HL> },
	/* 87 */ { &CPU::ADD_r_m1<Register8::A> },
	/* 88 */ { &CPU::ADC_r_m1<Register8::B> },
	/* 89 */ { &CPU::ADC_r_m1<Register8::C> },
	/* 8A */ { &CPU::ADC_r_m1<Register8::D> },
	/* 8B */ { &CPU::ADC_r_m1<Register8::E> },
	/* 8C */ { &CPU::ADC_r_m1<Register8::H> },
	/* 8D */ { &CPU::ADC_r_m1<Register8::L> },
	/* 8E */ { &CPU::ADC_arr_m1<Register16::HL>, &CPU::ADC_arr_m2<Register16::HL> },
	/* 8F */ { &CPU::ADC_r_m1<Register8::A> },
	/* 90 */ { &CPU::SUB_r_m1<Register8::B> },
	/* 91 */ { &CPU::SUB_r_m1<Register8::C> },
	/* 92 */ { &CPU::SUB_r_m1<Register8::D> },
	/* 93 */ { &CPU::SUB_r_m1<Register8::E> },
	/* 94 */ { &CPU::SUB_r_m1<Register8::H> },
	/* 95 */ { &CPU::SUB_r_m1<Register8::L> },
	/* 96 */ { &CPU::SUB_arr_m1<Register16::HL>, &CPU::SUB_arr_m2<Register16::HL> },
	/* 97 */ { &CPU::SUB_r_m1<Register8::A> },
	/* 98 */ { &CPU::SBC_r_m1<Register8::B> },
	/* 99 */ { &CPU::SBC_r_m1<Register8::C> },
	/* 9A */ { &CPU::SBC_r_m1<Register8::D> },
	/* 9B */ { &CPU::SBC_r_m1<Register8::E> },
	/* 9C */ { &CPU::SBC_r_m1<Register8::H> },
	/* 9D */ { &CPU::SBC_r_m1<Register8::L> },
	/* 9E */ { &CPU::SBC_arr_m1<Register16::HL>, &CPU::SBC_arr_m2<Register16::HL>  },
	/* 9F */ { &CPU::SBC_r_m1<Register8::A> },
	/* A0 */ { &CPU::AND_r_m1<Register8::B> },
	/* A1 */ { &CPU::AND_r_m1<Register8::C> },
	/* A2 */ { &CPU::AND_r_m1<Register8::D> },
	/* A3 */ { &CPU::AND_r_m1<Register8::E> },
	/* A4 */ { &CPU::AND_r_m1<Register8::H> },
	/* A5 */ { &CPU::AND_r_m1<Register8::L> },
	/* A6 */ { &CPU::AND_arr_m1<Register16::HL>, &CPU::AND_arr_m2<Register16::HL> },
	/* A7 */ { &CPU::AND_r_m1<Register8::A> },
	/* A8 */ { &CPU::XOR_r_m1<Register8::B> },
	/* A9 */ { &CPU::XOR_r_m1<Register8::C> },
	/* AA */ { &CPU::XOR_r_m1<Register8::D> },
	/* AB */ { &CPU::XOR_r_m1<Register8::E>  },
	/* AC */ { &CPU::XOR_r_m1<Register8::H>  },
	/* AD */ { &CPU::XOR_r_m1<Register8::L>  },
	/* AE */ { &CPU::XOR_arr_m1<Register16::HL>, &CPU::XOR_arr_m2<Register16::HL> },
	/* AF */ { &CPU::XOR_r_m1<Register8::A> },
	/* B0 */ { &CPU::OR_r_m1<Register8::B> },
	/* B1 */ { &CPU::OR_r_m1<Register8::C> },
	/* B2 */ { &CPU::OR_r_m1<Register8::D> },
	/* B3 */ { &CPU::OR_r_m1<Register8::E> },
	/* B4 */ { &CPU::OR_r_m1<Register8::H> },
	/* B5 */ { &CPU::OR_r_m1<Register8::L> },
	/* B6 */ { &CPU::OR_arr_m1<Register16::HL>, &CPU::OR_arr_m2<Register16::HL> },
	/* B7 */ { &CPU::OR_r_m1<Register8::A> },
	/* B8 */ { &CPU::CP_r_m1<Register8::B> },
	/* B9 */ { &CPU::CP_r_m1<Register8::C> },
	/* BA */ { &CPU::CP_r_m1<Register8::D> },
	/* BB */ { &CPU::CP_r_m1<Register8::E> },
	/* BC */ { &CPU::CP_r_m1<Register8::H> },
	/* BD */ { &CPU::CP_r_m1<Register8::L> },
	/* BE */ { &CPU::CP_arr_m1<Register16::HL>, &CPU::CP_arr_m2<Register16::HL> },
	/* BF */ { &CPU::CP_r_m1<Register8::A> },
	/* C0 */ { &CPU::instructionNotImplemented },
	/* C1 */ { &CPU::instructionNotImplemented },
	/* C2 */ { &CPU::JP_cc_nn_m1<Flag::N, Flag::Z>, &CPU::JP_cc_nn_m2<Flag::N, Flag::Z>, &CPU::JP_cc_nn_m3<Flag::N, Flag::Z>, &CPU::JP_cc_nn_m4<Flag::N, Flag::Z> },
	/* C3 */ { &CPU::JP_nn_m1, &CPU::JP_nn_m2, &CPU::JP_nn_m3, &CPU::JP_nn_m4 },
	/* C4 */ { &CPU::instructionNotImplemented },
	/* C5 */ { &CPU::instructionNotImplemented },
	/* C6 */ { &CPU::ADD_n_m1, &CPU::ADD_n_m2 },
	/* C7 */ { &CPU::instructionNotImplemented },
	/* C8 */ { &CPU::instructionNotImplemented },
	/* C9 */ { &CPU::instructionNotImplemented },
	/* CA */ { &CPU::JP_c_nn_m1<Flag::Z>, &CPU::JP_c_nn_m2<Flag::Z>, &CPU::JP_c_nn_m3<Flag::Z>, &CPU::JP_c_nn_m4<Flag::Z> },
	/* CB */ { &CPU::instructionNotImplemented },
	/* CC */ { &CPU::instructionNotImplemented },
	/* CD */ { &CPU::instructionNotImplemented },
	/* CE */ { &CPU::ADC_n_m1, &CPU::ADC_n_m2 },
	/* CF */ { &CPU::instructionNotImplemented },
	/* D0 */ { &CPU::instructionNotImplemented },
	/* D1 */ { &CPU::instructionNotImplemented },
	/* D2 */ { &CPU::JP_cc_nn_m1<Flag::N, Flag::C>, &CPU::JP_cc_nn_m2<Flag::N, Flag::C>, &CPU::JP_cc_nn_m3<Flag::N, Flag::C>, &CPU::JP_cc_nn_m4<Flag::N, Flag::C> },
	/* D3 */ { &CPU::instructionNotImplemented },
	/* D4 */ { &CPU::instructionNotImplemented },
	/* D5 */ { &CPU::instructionNotImplemented },
	/* D6 */ { &CPU::SUB_n_m1, &CPU::SUB_n_m2 },
	/* D7 */ { &CPU::instructionNotImplemented },
	/* D8 */ { &CPU::instructionNotImplemented },
	/* D9 */ { &CPU::instructionNotImplemented },
	/* DA */ { &CPU::JP_c_nn_m1<Flag::C>, &CPU::JP_c_nn_m2<Flag::C>, &CPU::JP_c_nn_m3<Flag::C>, &CPU::JP_c_nn_m4<Flag::C> },
	/* DB */ { &CPU::instructionNotImplemented },
	/* DC */ { &CPU::instructionNotImplemented },
	/* DD */ { &CPU::instructionNotImplemented },
	/* DE */ { &CPU::SBC_n_m1, &CPU::SBC_n_m2 },
	/* DF */ { &CPU::instructionNotImplemented },
	/* E0 */ { &CPU::LDH_an_r_m1<Register8::A>, &CPU::LDH_an_r_m2<Register8::A>, &CPU::LDH_an_r_m3<Register8::A> },
	/* E1 */ { &CPU::instructionNotImplemented },
	/* E2 */ { &CPU::LDH_ar_r_m1<Register8::C, Register8::A>, &CPU::LDH_ar_r_m2<Register8::C, Register8::A> },
	/* E3 */ { &CPU::instructionNotImplemented },
	/* E4 */ { &CPU::instructionNotImplemented },
	/* E5 */ { &CPU::instructionNotImplemented },
	/* E6 */ { &CPU::AND_n_m1, &CPU::AND_n_m2 },
	/* E7 */ { &CPU::instructionNotImplemented },
	/* E8 */ { &CPU::ADD_rr_n_m1<Register16::SP>, &CPU::ADD_rr_n_m2<Register16::SP>, &CPU::ADD_rr_n_m3<Register16::SP>, &CPU::ADD_rr_n_m4<Register16::SP> },
	/* E9 */ { &CPU::JP_rr_m1<Register16::HL> },
	/* EA */ { &CPU::LD_ann_r_m1<Register8::A>, &CPU::LD_ann_r_m2<Register8::A>, &CPU::LD_ann_r_m3<Register8::A>, &CPU::LD_ann_r_m4<Register8::A> },
	/* EB */ { &CPU::instructionNotImplemented },
	/* EC */ { &CPU::instructionNotImplemented },
	/* ED */ { &CPU::instructionNotImplemented },
	/* EE */ { &CPU::XOR_n_m1, &CPU::XOR_n_m2 },
	/* EF */ { &CPU::instructionNotImplemented },
	/* F0 */ { &CPU::LDH_r_an_m1<Register8::A>, &CPU::LDH_r_an_m2<Register8::A>, &CPU::LDH_r_an_m3<Register8::A> },
	/* F1 */ { &CPU::instructionNotImplemented },
	/* F2 */ { &CPU::LDH_r_ar_m1<Register8::A, Register8::C>, &CPU::LDH_r_ar_m2<Register8::A, Register8::C> },
	/* F3 */ { &CPU::DI_m1 },
	/* F4 */ { &CPU::instructionNotImplemented },
	/* F5 */ { &CPU::instructionNotImplemented },
	/* F6 */ { &CPU::OR_n_m1, &CPU::OR_n_m2 },
	/* F7 */ { &CPU::instructionNotImplemented },
	/* F8 */ { &CPU::LD_rr_rrn_m1<Register16::HL, Register16::SP>, &CPU::LD_rr_rrn_m2<Register16::HL, Register16::SP>, &CPU::LD_rr_rrn_m3<Register16::HL, Register16::SP> },
	/* F9 */ { &CPU::LD_rr_rr_m1<Register16::SP, Register16::HL>, &CPU::LD_rr_rr_m2<Register16::SP, Register16::HL> },
	/* FA */ { &CPU::LD_r_ann_m1<Register8::A>, &CPU::LD_r_ann_m2<Register8::A>, &CPU::LD_r_ann_m3<Register8::A>, &CPU::LD_r_ann_m4<Register8::A> },
	/* FB */ { &CPU::EI_m1 },
	/* FC */ { &CPU::instructionNotImplemented },
	/* FD */ { &CPU::instructionNotImplemented },
	/* FE */ { &CPU::CP_n_m1, &CPU::CP_n_m2 },
	/* FF */ { &CPU::instructionNotImplemented },
    } {

}

void CPU::reset() {
    mCycles = 0;
    PC = 0x100;
    SP = 0;
    AF = 0;
    BC = 0;
    DE = 0;
    HL = 0;
    currentInstruction.opcode = 0x00;
    currentInstruction.microop = 0;
    u1 = 0;
    u2 = 0;
}

void CPU::tick() {
    DEBUG(2)
        << "------- CPU:tick -------\n"
        << status() << "\n"
        << std::endl;


    DEBUG(2) << ">> " << hex(currentInstruction.opcode) << " [MicroOp=" << std::to_string(currentInstruction.microop + 1) << "]" << std::endl;

    auto micro_op = instructions[currentInstruction.opcode][currentInstruction.microop++];
    if (!micro_op)
        throw IllegalInstructionException(
                "Illegal instruction: " + hex(currentInstruction.opcode) +
                " [MicroOp=" + std::to_string(currentInstruction.microop + 1) + "]");
    (this->*micro_op)();

    mCycles++;

    DEBUG(2)
        << "\n"
        << status() << "\n"
        << "---------------------\n" << std::endl;
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::A>() const {
    return get_byte<1>(AF);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::B>() const {
    return get_byte<1>(BC);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::C>() const {
    return get_byte<0>(BC);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::D>() const {
    return get_byte<1>(DE);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::E>() const {
    return get_byte<0>(DE);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::F>() const {
    return get_byte<0>(AF);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::H>() const {
    return get_byte<1>(HL);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::L>() const {
    return get_byte<0>(HL);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::S>() const {
    return get_byte<1>(SP);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::P>() const {
    return get_byte<0>(SP);
}

template<>
void CPU::writeRegister8<CPU::Register8::A>(uint8_t value) {
    return set_byte<1>(AF, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::B>(uint8_t value) {
    return set_byte<1>(BC, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::C>(uint8_t value) {
    return set_byte<0>(BC, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::D>(uint8_t value) {
    return set_byte<1>(DE, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::E>(uint8_t value) {
    return set_byte<0>(DE, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::F>(uint8_t value) {
    return set_byte<0>(AF, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::H>(uint8_t value) {
    return set_byte<1>(HL, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::L>(uint8_t value) {
    return set_byte<0>(HL, value);
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::AF>() const {
    return AF;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::BC>() const {
    return BC;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::DE>() const {
    return DE;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::HL>() const {
    return HL;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::PC>() const {
    return PC;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::SP>() const {
    return SP;
}

template<>
void CPU::writeRegister16<CPU::Register16::AF>(uint16_t value) {
    AF = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::BC>(uint16_t value) {
    BC = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::DE>(uint16_t value) {
    DE = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::HL>(uint16_t value) {
    HL = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::PC>(uint16_t value) {
    PC = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::SP>(uint16_t value) {
    SP = value;
}

template<>
[[nodiscard]] bool CPU::readFlag<CPU::Flag::Z>() const {
    return get_bit<7>(AF);
}

template<>
[[nodiscard]] bool CPU::readFlag<CPU::Flag::N>() const {
    return get_bit<6>(AF);
}

template<>
[[nodiscard]] bool CPU::readFlag<CPU::Flag::H>() const {
    return get_bit<5>(AF);
}

template<>
[[nodiscard]] bool CPU::readFlag<CPU::Flag::C>() const {
    return get_bit<4>(AF);
}

template<>
void CPU::writeFlag<CPU::Flag::Z>(bool value) {
    set_bit<7>(AF, value);
}

template<>
void CPU::writeFlag<CPU::Flag::N>(bool value) {
    set_bit<6>(AF, value);
}

template<>
void CPU::writeFlag<CPU::Flag::H>(bool value) {
    set_bit<5>(AF, value);
}

template<>
void CPU::writeFlag<CPU::Flag::C>(bool value) {
    set_bit<4>(AF, value);
}

std::string CPU::status() const {
    std::stringstream ss;
    ss  << " PC  "
        << " SP  "
        << "   A        F     "
        << "   B        C     "
        << "   D        E     "
        << "   H         L"
        << "\n"
        << hex(PC) << " "
        << hex(SP) << " "
        << bin(readRegister8<Register8::A>()) << " " << bin(readRegister8<Register8::F>()) << " "
        << bin(readRegister8<Register8::B>()) << " " << bin(readRegister8<Register8::C>()) << " "
        << bin(readRegister8<Register8::D>()) << " " << bin(readRegister8<Register8::E>()) << " "
        << bin(readRegister8<Register8::H>()) << " " << bin(readRegister8<Register8::L>()) << " "
        << "\n"
        << "instruction  "
        << "M-cycles"
        << "\n"
        << "  " << hex(currentInstruction.opcode) << "[M=" << std::to_string(currentInstruction.microop + 1) << "]      "
        << " " << mCycles
    ;
    return ss.str();
}

// ============================= INSTRUCTIONS ==================================

void CPU::fetch() {
    DEBUG(2) << "<fetch>" << std::endl;
    currentInstruction.opcode = bus.read(PC++);
    currentInstruction.microop = 0;
}

void CPU::instructionNotImplemented() {
    DEBUG(2) << "<not implemented>" << std::endl;
    throw InstructionNotImplementedException(
            "Instruction not implemented: " + hex(currentInstruction.opcode) +
            " [MicroOp=" + std::to_string(currentInstruction.microop + 1) + "]");
}

void CPU::NOP_m1() {
    fetch();
}

void CPU::STOP_m1() {
    // TODO: what to do here?
    fetch();
}

void CPU::HALT_m1() {
    // TODO: what to do here?
}

void CPU::DI_m1() {
    IME = false;
    fetch();
}

void CPU::EI_m1() {
    IME = true;
    fetch();
}


// e.g. 01 | LD BC,d16

template<CPU::Register16 rr>
void CPU::LD_rr_nn_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_rr_nn_m2() {
    u2 = bus.read(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_rr_nn_m3() {
    writeRegister16<rr>(concat_bytes(u2, u1));
    fetch();
}

// e.g. 36 | LD (HL),d8

template<CPU::Register16 rr>
void CPU::LD_arr_n_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_arr_n_m2() {
    bus.write(readRegister16<rr>(), u1);
}

template<CPU::Register16 rr>
void CPU::LD_arr_n_m3() {
    fetch();
}

// e.g. 02 | LD (BC),A

template<CPU::Register16 rr, CPU::Register8 r>
void CPU::LD_arr_r_m1() {
    bus.write(readRegister16<rr>(), readRegister8<r>());
}

template<CPU::Register16 rr, CPU::Register8 r>
void CPU::LD_arr_r_m2() {
    fetch();
}

// e.g. 22 | LD (HL+),A

template<CPU::Register16 rr, CPU::Register8 r, int8_t inc>
void CPU::LD_arr_r_m1() {
    bus.write(readRegister16<rr>(), readRegister8<r>());
    writeRegister16<rr>(readRegister16<rr>() + inc);
}

template<CPU::Register16 rr, CPU::Register8 r, int8_t inc>
void CPU::LD_arr_r_m2() {
    fetch();
}

// e.g. 06 | LD B,d8

template<CPU::Register8 r>
void CPU::LD_r_n_m1() {
    writeRegister8<r>(bus.read(PC++));
}

template<CPU::Register8 r>
void CPU::LD_r_n_m2() {
    fetch();
}

// e.g. 08 | LD (a16),SP

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m2() {
    u2 = bus.read(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m3() {
    uu1 = concat_bytes(u2, u1);
    bus.write(uu1, readRegister8<Register8::P>());
    // TODO: P or S?
}

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m4() {
    bus.write(uu1 + 1, readRegister8<Register8::S>());
    // TODO: P or S?
}

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m5() {
    fetch();
}

// e.g. 0A |  LD A,(BC)

template<CPU::Register8 r,  CPU::Register16 rr>
void CPU::LD_r_arr_m1() {
    writeRegister8<r>(bus.read(readRegister16<rr>()));
}

template<CPU::Register8 r, CPU::Register16 rr>
void CPU::LD_r_arr_m2() {
    fetch();
}

// e.g. 2A |  LD A,(HL+)

template<CPU::Register8 r,  CPU::Register16 rr, int8_t inc>
void CPU::LD_r_arr_m1() {
    writeRegister8<r>(bus.read(readRegister16<rr>()));
    writeRegister16<rr>(readRegister16<rr>() + inc);
}

template<CPU::Register8 r, CPU::Register16 rr, int8_t inc>
void CPU::LD_r_arr_m2() {
    fetch();
}

// e.g. 41 |  LD B,C

template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LD_r_r_m1() {
    writeRegister8<r1>(readRegister8<r2>());
    fetch();
}

// e.g. E0 | LDH (a8),A

template<CPU::Register8 r>
void CPU::LDH_an_r_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Register8 r>
void CPU::LDH_an_r_m2() {
    bus.write(concat_bytes(0xFF, u1), readRegister8<r>());
}

template<CPU::Register8 r>
void CPU::LDH_an_r_m3() {
    fetch();
}

// e.g. F0 | LDH A,(a8)

template<CPU::Register8 r>
void CPU::LDH_r_an_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Register8 r>
void CPU::LDH_r_an_m2() {
    writeRegister8<r>(bus.read(concat_bytes(0xFF, u1)));
}

template<CPU::Register8 r>
void CPU::LDH_r_an_m3() {
    fetch();
}

// e.g. E2 | LD (C),A

template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LDH_ar_r_m1() {
    bus.write(concat_bytes(0xFF, readRegister8<r1>()), readRegister8<r2>());
}

template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LDH_ar_r_m2() {
    fetch();
}

// e.g. F2 | LD A,(C)

template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LDH_r_ar_m1() {
    writeRegister8<r1>(bus.read(concat_bytes(0xFF, readRegister8<r2>())));
}


template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LDH_r_ar_m2() {
    fetch();
}

// e.g. EA | LD (a16),A

template<CPU::Register8 r>
void CPU::LD_ann_r_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Register8 r>
void CPU::LD_ann_r_m2() {
    u2 = bus.read(PC++);
}

template<CPU::Register8 r>
void CPU::LD_ann_r_m3() {
    bus.write(concat_bytes(u2, u1), readRegister8<r>());
}

template<CPU::Register8 r>
void CPU::LD_ann_r_m4() {
    fetch();
}

// e.g. FA | LD A,(a16)

template<CPU::Register8 r>
void CPU::LD_r_ann_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Register8 r>
void CPU::LD_r_ann_m2() {
    u2 = bus.read(PC++);
}

template<CPU::Register8 r>
void CPU::LD_r_ann_m3() {
    writeRegister8<r>(bus.read(concat_bytes(u2, u1)));
}

template<CPU::Register8 r>
void CPU::LD_r_ann_m4() {
    fetch();
}

// e.g. F9 | LD SP,HL

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rr_m1() {
    writeRegister16<rr1>(readRegister16<rr2>());
    // TODO: i guess this should be split in the two 8bit registers between m1 and m2
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rr_m2() {
    fetch();
}

// e.g. F8 | LD HL,SP+r8


template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrn_m1() {
    u1 = (int8_t) bus.read(PC++);
    // TODO: when are flags set?
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrn_m2() {
    uu1 = readRegister16<rr2>();
    auto [result, h, c] = sum_carry<3, 7>(uu1, u1);
    writeRegister16<rr1>(result);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    // TODO: does it work?
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrn_m3() {
    fetch();
}

// e.g. 04 | INC B

template<CPU::Register8 r>
void CPU::INC_r_m1() {
    uint8_t tmp = readRegister8<r>();
    auto [result, h] = sum_carry<3>(tmp, 1);
    writeRegister8<r>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    fetch();
}


// e.g. 03 | INC BC

template<CPU::Register16 rr>
void CPU::INC_rr_m1() {
    uint16_t tmp = readRegister16<rr>();
    uint32_t result = tmp + 1;
    writeRegister16<rr>(result);
    // TODO: no flags?
}

template<CPU::Register16 rr>
void CPU::INC_rr_m2() {
    fetch();
}

// e.g. 34 | INC (HL)

template<CPU::Register16 rr>
void CPU::INC_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::INC_arr_m2() {
    auto [result, h] = sum_carry<3>(u1, 1);
    bus.write(readRegister16<rr>(), result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
}

template<CPU::Register16 rr>
void CPU::INC_arr_m3() {
    fetch();
}

// e.g. 05 | DEC B

template<CPU::Register8 r>
void CPU::DEC_r_m1() {
    uint8_t tmp = readRegister8<r>();
    auto [result, h] = sum_carry<3>(tmp, -1);
    writeRegister8<r>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    fetch();
}


// e.g. 0B | DEC BC

template<CPU::Register16 rr>
void CPU::DEC_rr_m1() {
    uint16_t tmp = readRegister16<rr>();
    uint32_t result = tmp - 1;
    writeRegister16<rr>(result);
    // TODO: no flags?
}

template<CPU::Register16 rr>
void CPU::DEC_rr_m2() {
    fetch();
}

// e.g. 35 | DEC (HL)

template<CPU::Register16 rr>
void CPU::DEC_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::DEC_arr_m2() {
    auto [result, h] = sum_carry<3>(u1, -1);
    bus.write(readRegister16<rr>(), result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
}

template<CPU::Register16 rr>
void CPU::DEC_arr_m3() {
    fetch();
}

// e.g. 80 | ADD B

template<CPU::Register8 r>
void CPU::ADD_r_m1() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 86 | ADD (HL)

template<CPU::Register16 rr>
void CPU::ADD_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::ADD_arr_m2() {
    // TODO: dont like this + C very much...
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), u1);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// C6 | ADD A,d8

void CPU::ADD_n_m1() {
    u1 = bus.read(PC++);
}

void CPU::ADD_n_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), u1);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 88 | ADC B

template<CPU::Register8 r>
void CPU::ADC_r_m1() {
    // TODO: dont like this + C very much...
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), readRegister8<r>() + readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 8E | ADC (HL)

template<CPU::Register16 rr>
void CPU::ADC_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::ADC_arr_m2() {
    // TODO: dont like this + C very much...
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), u1 + readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// CE | ADC A,d8

void CPU::ADC_n_m1() {
    u1 = bus.read(PC++);
}

void CPU::ADC_n_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), u1 + readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 09 | ADD HL,BC

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::ADD_rr_rr_m1() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister16<rr1>(), readRegister16<rr2>());
    writeRegister16<rr1>(result);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::ADD_rr_rr_m2() {
    fetch();
}

// e.g. E8 | ADD SP,r8

template<CPU::Register16 rr>
void CPU::ADD_rr_n_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Register16 rr>
void CPU::ADD_rr_n_m2() {
   auto [result, h, c] = sum_carry<3, 7>(readRegister16<rr>(), u1);
    writeRegister16<rr>(result);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::H>(c);
}


template<CPU::Register16 rr>
void CPU::ADD_rr_n_m3() {
    // TODO: why? i guess something about the instruction timing is wrong
}


template<CPU::Register16 rr>
void CPU::ADD_rr_n_m4() {
    fetch();
}


// e.g. 90 | SUB B

template<CPU::Register8 r>
void CPU::SUB_r_m1() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), -readRegister8<r>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 96 | SUB (HL)

template<CPU::Register16 rr>
void CPU::SUB_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::SUB_arr_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), -u1);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// D6 | SUB A,d8

void CPU::SUB_n_m1() {
    u1 = bus.read(PC++);
}

void CPU::SUB_n_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), -u1);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 98 | SBC B

template<CPU::Register8 r>
void CPU::SBC_r_m1() {
    // TODO: dont like this - C very much...
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), - readRegister8<r>() - readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 9E | SBC A,(HL)

template<CPU::Register16 rr>
void CPU::SBC_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::SBC_arr_m2() {
    // TODO: dont like this - C very much...
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), - u1 - readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// D6 | SBC A,d8

void CPU::SBC_n_m1() {
    u1 = bus.read(PC++);
}

void CPU::SBC_n_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), - u1 - readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. A0 | AND B

template<CPU::Register8 r>
void CPU::AND_r_m1() {
    uint8_t result = readRegister8<Register8::A>() & readRegister8<r>();
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}

// e.g. A6 | AND (HL)

template<CPU::Register16 rr>
void CPU::AND_arr_m1() {
    u1 = readRegister16<rr>();
}

template<CPU::Register16 rr>
void CPU::AND_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() & u1;
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}

// E6 | AND d8

void CPU::AND_n_m1() {
    u1 = bus.read(PC++);
}

void CPU::AND_n_m2() {
    uint8_t result = readRegister8<Register8::A>() & u1;
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}


// B0 | OR B

template<CPU::Register8 r>
void CPU::OR_r_m1() {
    uint8_t result = readRegister8<Register8::A>() | readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

// e.g. B6 | OR (HL)

template<CPU::Register16 rr>
void CPU::OR_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::OR_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() | u1;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


// F6 | OR d8

void CPU::OR_n_m1() {
    u1 = bus.read(PC++);
}

void CPU::OR_n_m2() {
    uint8_t result = readRegister8<Register8::A>() | u1;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


// e.g. A8 | XOR B

template<CPU::Register8 r>
void CPU::XOR_r_m1() {
    uint8_t result = readRegister8<Register8::A>() ^ readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

template<CPU::Register16 rr>
void CPU::XOR_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::XOR_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() ^ u1;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

// EE | XOR d8

void CPU::XOR_n_m1() {
    u1 = bus.read(PC++);
}

void CPU::XOR_n_m2() {
    uint8_t result = readRegister8<Register8::A>() ^ u1;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


template<CPU::Register8 r>
void CPU::CP_r_m1() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), -readRegister8<r>());
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

template<CPU::Register16 rr>
void CPU::CP_arr_m1() {
    u1 = bus.read(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::CP_arr_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), -u1);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// FE | CP d8

void CPU::CP_n_m1() {
    u1 = bus.read(PC++);
}

void CPU::CP_n_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), -u1);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// 27 | DAA

void CPU::DAA_m1() {
    uint8_t a = readRegister8<Register8::A>();

    // TODO: flags are wrong for sure
    writeFlag<Flag::H>(false);
    writeFlag<Flag::H>(false);

    if ((a & 0x0F) > 9) {
        auto [result, h, c] = sum_carry<3, 7>(a, 0x06);
        writeFlag<Flag::H>(h);
        writeFlag<Flag::H>(c);
        a = result;
    }
    if ((a & 0xF0) > 9) {
        auto [result, h, c] = sum_carry<3, 7>(a, 0x60);
        writeFlag<Flag::H>(h);
        writeFlag<Flag::H>(c);
        a = result;
    }
    writeRegister8<Register8::A>(a);
    writeFlag<Flag::N>(false);
    fetch();
}

// 37 | SCF

void CPU::SCF_m1() {
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(true);
    fetch();
}

// 2F | CPL

void CPU::CPL_m1() {
    writeRegister8<Register8::A>(~readRegister8<Register8::A>());
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(true);
    fetch();
}

// 3F | CCF_m1

void CPU::CCF_m1() {
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(!readFlag<Flag::C>());
    fetch();
}

// 07 | RLCA

void CPU::RLCA_m1() {
    uint8_t c = readFlag<Flag::C>();
    uint8_t a = readRegister8<Register8::A>();
    writeFlag<Flag::C>(get_bit<7>(a));
    a = (a << 1) | c;
    writeRegister8<Register8::A>(a);
    fetch();
}

// 17 | RLA

void CPU::RLA_m1() {
    uint8_t a = readRegister8<Register8::A>();
    writeFlag<Flag::C>(get_bit<7>(a));
    a = (a << 1) | (get_bit<7>(a));
    writeRegister8<Register8::A>(a);
    fetch();
}

// 0F | RRCA

void CPU::RRCA_m1() {
    uint8_t c = readFlag<Flag::C>();
    uint8_t a = readRegister8<Register8::A>();
    writeFlag<Flag::C>(get_bit<0>(a));
    a = (a >> 1) | (c << 7);
    writeRegister8<Register8::A>(a);
    fetch();
}

// 1F | RRA

void CPU::RRA_m1() {
    uint8_t a = readRegister8<Register8::A>();
    writeFlag<Flag::C>(get_bit<0>(a));
    a = (a >> 1) | (get_bit<0>(a) << 7);
    writeRegister8<Register8::A>(a);
    fetch();
}

// e.g. 18 | JR r8

void  CPU::JR_n_m1() {
    u1 = bus.read(PC++);
}

void  CPU::JR_n_m2() {
    PC += u1;
}

void  CPU::JR_n_m3() {
    fetch();
}

// e.g. 28 | JR Z,r8

template<CPU::Flag f>
void CPU::JR_c_n_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Flag f>
void CPU::JR_c_n_m2() {
    if (readFlag<f>()) {
        PC += u1;
    } else {
        fetch();
    }
}

template<CPU::Flag f>
void CPU::JR_c_n_m3() {
    fetch();
}


// e.g. 20 | JR NZ,r8

template<CPU::Flag f1, CPU::Flag f2>
void CPU::JR_cc_n_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Flag f1, CPU::Flag f2>
void CPU::JR_cc_n_m2() {
    if (readFlag<f1>() && readFlag<f2>()) {
        PC += u1;
    } else {
        fetch();
    }
}

template<CPU::Flag f1, CPU::Flag f2>
void CPU::JR_cc_n_m3() {
    fetch();
}

// e.g. C3 | JP a16

void  CPU::JP_nn_m1() {
    u1 = bus.read(PC++);
}

void CPU::JP_nn_m2() {
    u2 = bus.read(PC++);
}

void CPU::JP_nn_m3() {
    PC = concat_bytes(u2, u1);
}

void CPU::JP_nn_m4() {
    fetch();
}

// e.g. E9 | JP (HL)

template<CPU::Register16 rr>
void CPU::JP_rr_m1() {
    PC = readRegister16<rr>();
    fetch();
}

// e.g. CA | JP Z,a16

template<CPU::Flag f>
void CPU::JP_c_nn_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Flag f>
void CPU::JP_c_nn_m2() {
    u2 = bus.read(PC++);
}

template<CPU::Flag f>
void CPU::JP_c_nn_m3() {
    if (readFlag<f>()) {
        PC = concat_bytes(u2, u1);
    } else {
        fetch();
    }
}

template<CPU::Flag f>
void CPU::JP_c_nn_m4() {
    fetch();
}


// e.g. C2 | JP NZ,a16

template<CPU::Flag f1, CPU::Flag f2>
void CPU::JP_cc_nn_m1() {
    u1 = bus.read(PC++);
}

template<CPU::Flag f1, CPU::Flag f2>
void CPU::JP_cc_nn_m2() {
    u2 = bus.read(PC++);
}

template<CPU::Flag f1, CPU::Flag f2>
void CPU::JP_cc_nn_m3() {
    if (readFlag<f1>() && readFlag<f2>()) {
        PC = concat_bytes(u2, u1);
    } else {
        fetch();
    }
}

template<CPU::Flag f1, CPU::Flag f2>
void CPU::JP_cc_nn_m4() {
    fetch();
}

// ----

uint16_t CPU::getAF() const {
    return AF;
}

uint16_t CPU::getBC() const {
    return BC;
}

uint16_t CPU::getDE() const {
    return DE;
}

uint16_t CPU::getHL() const {
    return HL;
}

uint16_t CPU::getPC() const {
    return PC;
}

uint16_t CPU::getSP() const {
    return SP;
}

uint8_t CPU::getCurrentInstructionOpcode() const {
    return currentInstruction.opcode;
}

uint8_t CPU::getCurrentInstructionMicroOperation() const {
    return currentInstruction.microop;
}