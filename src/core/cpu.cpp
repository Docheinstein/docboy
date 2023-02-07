#include "cpu.h"
#include "log/log.h"
#include "utils/binutils.h"
#include <functional>
#include <cassert>

CPU::InstructionNotImplementedException::InstructionNotImplementedException(const std::string &what) : logic_error(what) {

}
CPU::IllegalInstructionException::IllegalInstructionException(const std::string &what) : logic_error(what) {

}

CPU::CPU(Bus &bus) :
        bus(bus),
        mCycles(0),
        PC(0x100),
        SP(0),
        AF(0),
        BC(0),
        DE(0),
        HL(0),
        currentInstruction(),
        u1(),
        u2(),
        instructions {

	/* 00 */ { &CPU::NOP_m1 },
	/* 01 */ { &CPU::LD_rr_nn_m1<Register16::BC>, &CPU::LD_rr_nn_m2<Register16::BC>, &CPU::LD_rr_nn_m3<Register16::BC> },
	/* 02 */ { &CPU::LD_arr_r_m1<Register16::BC, Register8::A>, &CPU::LD_arr_r_m2<Register16::BC, Register8::A> },
	/* 03 */ { &CPU::instructionNotImplemented },
	/* 04 */ { &CPU::instructionNotImplemented },
	/* 05 */ { &CPU::instructionNotImplemented },
	/* 06 */ { &CPU::LD_r_n_m1<Register8::B>, &CPU::LD_r_n_m2<Register8::B> },
	/* 07 */ { &CPU::instructionNotImplemented },
	/* 08 */ { &CPU::LD_ann_rr_m1<Register16::SP>, &CPU::LD_ann_rr_m2<Register16::SP>, &CPU::LD_ann_rr_m3<Register16::SP>, &CPU::LD_ann_rr_m4<Register16::SP>, &CPU::LD_ann_rr_m5<Register16::SP> },
	/* 09 */ { &CPU::instructionNotImplemented },
	/* 0A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::BC>, &CPU::LD_r_arr_m2<Register8::A, Register16::BC> },
	/* 0B */ { &CPU::instructionNotImplemented },
	/* 0C */ { &CPU::instructionNotImplemented },
	/* 0D */ { &CPU::instructionNotImplemented },
	/* 0E */ { &CPU::LD_r_n_m1<Register8::C>, &CPU::LD_r_n_m2<Register8::C> },
	/* 0F */ { &CPU::instructionNotImplemented },
	/* 10 */ { &CPU::instructionNotImplemented },
	/* 11 */ { &CPU::LD_rr_nn_m1<Register16::DE>, &CPU::LD_rr_nn_m2<Register16::DE>, &CPU::LD_rr_nn_m3<Register16::DE> },
	/* 12 */ { &CPU::LD_arr_r_m1<Register16::DE, Register8::A>, &CPU::LD_arr_r_m2<Register16::DE, Register8::A> },
	/* 13 */ { &CPU::instructionNotImplemented },
	/* 14 */ { &CPU::instructionNotImplemented },
	/* 15 */ { &CPU::instructionNotImplemented },
	/* 16 */ { &CPU::LD_r_n_m1<Register8::D>, &CPU::LD_r_n_m2<Register8::D> },
	/* 17 */ { &CPU::instructionNotImplemented },
	/* 18 */ { &CPU::instructionNotImplemented },
	/* 19 */ { &CPU::instructionNotImplemented },
	/* 1A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::DE>, &CPU::LD_r_arr_m2<Register8::A, Register16::DE> },
	/* 1B */ { &CPU::instructionNotImplemented },
	/* 1C */ { &CPU::instructionNotImplemented },
	/* 1D */ { &CPU::instructionNotImplemented },
	/* 1E */ { &CPU::LD_r_n_m1<Register8::E>, &CPU::LD_r_n_m2<Register8::E> },
	/* 1F */ { &CPU::instructionNotImplemented },
	/* 20 */ { &CPU::instructionNotImplemented },
	/* 21 */ { &CPU::LD_rr_nn_m1<Register16::HL>, &CPU::LD_rr_nn_m2<Register16::HL>, &CPU::LD_rr_nn_m3<Register16::HL> },
	/* 22 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::A, 1>, &CPU::LD_arr_r_m2<Register16::HL, Register8::A, 1> },
	/* 23 */ { &CPU::instructionNotImplemented },
	/* 24 */ { &CPU::instructionNotImplemented },
	/* 25 */ { &CPU::instructionNotImplemented },
	/* 26 */ { &CPU::LD_r_n_m1<Register8::H>, &CPU::LD_r_n_m2<Register8::H> },
	/* 27 */ { &CPU::instructionNotImplemented },
	/* 28 */ { &CPU::instructionNotImplemented },
	/* 29 */ { &CPU::instructionNotImplemented },
	/* 2A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::HL, 1>, &CPU::LD_r_arr_m2<Register8::A, Register16::HL, 1> },
	/* 2B */ { &CPU::instructionNotImplemented },
	/* 2C */ { &CPU::instructionNotImplemented },
	/* 2D */ { &CPU::instructionNotImplemented },
	/* 2E */ { &CPU::LD_r_n_m1<Register8::L>, &CPU::LD_r_n_m2<Register8::L> },
	/* 2F */ { &CPU::instructionNotImplemented },
	/* 30 */ { &CPU::instructionNotImplemented },
	/* 31 */ { &CPU::LD_rr_nn_m1<Register16::SP>, &CPU::LD_rr_nn_m2<Register16::SP>, &CPU::LD_rr_nn_m3<Register16::SP> },
	/* 32 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::A, -1>, &CPU::LD_arr_r_m2<Register16::HL, Register8::A, -1> },
	/* 33 */ { &CPU::instructionNotImplemented },
	/* 34 */ { &CPU::instructionNotImplemented },
	/* 35 */ { &CPU::instructionNotImplemented },
	/* 36 */ { &CPU::instructionNotImplemented },
	/* 37 */ { &CPU::instructionNotImplemented },
	/* 38 */ { &CPU::instructionNotImplemented },
	/* 39 */ { &CPU::instructionNotImplemented },
	/* 3A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::HL, -1>, &CPU::LD_r_arr_m2<Register8::A, Register16::HL, -1> },
	/* 3B */ { &CPU::instructionNotImplemented },
	/* 3C */ { &CPU::instructionNotImplemented },
	/* 3D */ { &CPU::instructionNotImplemented },
	/* 3E */ { &CPU::LD_r_n_m1<Register8::A>, &CPU::LD_r_n_m2<Register8::A> },
	/* 3F */ { &CPU::instructionNotImplemented },
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
	/* 4F */ { &CPU::instructionNotImplemented },
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
	/* 76 */ { &CPU::instructionNotImplemented },
	/* 77 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::A>, &CPU::LD_arr_r_m2<Register16::HL, Register8::A> },
	/* 78 */ { &CPU::LD_r_r_m1<Register8::A, Register8::B> },
	/* 79 */ { &CPU::LD_r_r_m1<Register8::A, Register8::C> },
	/* 7A */ { &CPU::LD_r_r_m1<Register8::A, Register8::D> },
	/* 7B */ { &CPU::LD_r_r_m1<Register8::A, Register8::E> },
	/* 7C */ { &CPU::LD_r_r_m1<Register8::A, Register8::H> },
	/* 7D */ { &CPU::LD_r_r_m1<Register8::A, Register8::L> },
	/* 7E */ { &CPU::LD_r_arr_m1<Register8::A, Register16::HL>, &CPU::LD_r_arr_m2<Register8::A, Register16::HL> },
	/* 7F */ { &CPU::LD_r_r_m1<Register8::A, Register8::A> },
	/* 80 */ { &CPU::instructionNotImplemented },
	/* 81 */ { &CPU::instructionNotImplemented },
	/* 82 */ { &CPU::instructionNotImplemented },
	/* 83 */ { &CPU::instructionNotImplemented },
	/* 84 */ { &CPU::instructionNotImplemented },
	/* 85 */ { &CPU::instructionNotImplemented },
	/* 86 */ { &CPU::instructionNotImplemented },
	/* 87 */ { &CPU::instructionNotImplemented },
	/* 88 */ { &CPU::instructionNotImplemented },
	/* 89 */ { &CPU::instructionNotImplemented },
	/* 8A */ { &CPU::instructionNotImplemented },
	/* 8B */ { &CPU::instructionNotImplemented },
	/* 8C */ { &CPU::instructionNotImplemented },
	/* 8D */ { &CPU::instructionNotImplemented },
	/* 8E */ { &CPU::instructionNotImplemented },
	/* 8F */ { &CPU::instructionNotImplemented },
	/* 90 */ { &CPU::instructionNotImplemented },
	/* 91 */ { &CPU::instructionNotImplemented },
	/* 92 */ { &CPU::instructionNotImplemented },
	/* 93 */ { &CPU::instructionNotImplemented },
	/* 94 */ { &CPU::instructionNotImplemented },
	/* 95 */ { &CPU::instructionNotImplemented },
	/* 96 */ { &CPU::instructionNotImplemented },
	/* 97 */ { &CPU::instructionNotImplemented },
	/* 98 */ { &CPU::instructionNotImplemented },
	/* 99 */ { &CPU::instructionNotImplemented },
	/* 9A */ { &CPU::instructionNotImplemented },
	/* 9B */ { &CPU::instructionNotImplemented },
	/* 9C */ { &CPU::instructionNotImplemented },
	/* 9D */ { &CPU::instructionNotImplemented },
	/* 9E */ { &CPU::instructionNotImplemented },
	/* 9F */ { &CPU::instructionNotImplemented },
	/* A0 */ { &CPU::instructionNotImplemented },
	/* A1 */ { &CPU::instructionNotImplemented },
	/* A2 */ { &CPU::instructionNotImplemented },
	/* A3 */ { &CPU::instructionNotImplemented },
	/* A4 */ { &CPU::instructionNotImplemented },
	/* A5 */ { &CPU::instructionNotImplemented },
	/* A6 */ { &CPU::instructionNotImplemented },
	/* A7 */ { &CPU::instructionNotImplemented },
	/* A8 */ { &CPU::XOR_r_m1<Register8::B> },
	/* A9 */ { &CPU::XOR_r_m1<Register8::C> },
	/* AA */ { &CPU::XOR_r_m1<Register8::D> },
	/* AB */ { &CPU::XOR_r_m1<Register8::E>  },
	/* AC */ { &CPU::XOR_r_m1<Register8::H>  },
	/* AD */ { &CPU::XOR_r_m1<Register8::L>  },
	/* AE */ { &CPU::XOR_arr_m1<Register16::HL>, &CPU::XOR_arr_m2<Register16::HL> },
	/* AF */ { &CPU::XOR_r_m1<Register8::A> },
	/* B0 */ { &CPU::instructionNotImplemented },
	/* B1 */ { &CPU::instructionNotImplemented },
	/* B2 */ { &CPU::instructionNotImplemented },
	/* B3 */ { &CPU::instructionNotImplemented },
	/* B4 */ { &CPU::instructionNotImplemented },
	/* B5 */ { &CPU::instructionNotImplemented },
	/* B6 */ { &CPU::instructionNotImplemented },
	/* B7 */ { &CPU::instructionNotImplemented },
	/* B8 */ { &CPU::instructionNotImplemented },
	/* B9 */ { &CPU::instructionNotImplemented },
	/* BA */ { &CPU::instructionNotImplemented },
	/* BB */ { &CPU::instructionNotImplemented },
	/* BC */ { &CPU::instructionNotImplemented },
	/* BD */ { &CPU::instructionNotImplemented },
	/* BE */ { &CPU::instructionNotImplemented },
	/* BF */ { &CPU::instructionNotImplemented },
	/* C0 */ { &CPU::instructionNotImplemented },
	/* C1 */ { &CPU::instructionNotImplemented },
	/* C2 */ { &CPU::instructionNotImplemented },
	/* C3 */ { &CPU::JP_nn_m1, &CPU::JP_nn_m2, &CPU::JP_nn_m3, &CPU::JP_nn_m4 },
	/* C4 */ { &CPU::instructionNotImplemented },
	/* C5 */ { &CPU::instructionNotImplemented },
	/* C6 */ { &CPU::instructionNotImplemented },
	/* C7 */ { &CPU::instructionNotImplemented },
	/* C8 */ { &CPU::instructionNotImplemented },
	/* C9 */ { &CPU::instructionNotImplemented },
	/* CA */ { &CPU::instructionNotImplemented },
	/* CB */ { &CPU::instructionNotImplemented },
	/* CC */ { &CPU::instructionNotImplemented },
	/* CD */ { &CPU::instructionNotImplemented },
	/* CE */ { &CPU::instructionNotImplemented },
	/* CF */ { &CPU::instructionNotImplemented },
	/* D0 */ { &CPU::instructionNotImplemented },
	/* D1 */ { &CPU::instructionNotImplemented },
	/* D2 */ { &CPU::instructionNotImplemented },
	/* D3 */ { &CPU::instructionNotImplemented },
	/* D4 */ { &CPU::instructionNotImplemented },
	/* D5 */ { &CPU::instructionNotImplemented },
	/* D6 */ { &CPU::instructionNotImplemented },
	/* D7 */ { &CPU::instructionNotImplemented },
	/* D8 */ { &CPU::instructionNotImplemented },
	/* D9 */ { &CPU::instructionNotImplemented },
	/* DA */ { &CPU::instructionNotImplemented },
	/* DB */ { &CPU::instructionNotImplemented },
	/* DC */ { &CPU::instructionNotImplemented },
	/* DD */ { &CPU::instructionNotImplemented },
	/* DE */ { &CPU::instructionNotImplemented },
	/* DF */ { &CPU::instructionNotImplemented },
	/* E0 */ { &CPU::LDH_an_r_m1<Register8::A>, &CPU::LDH_an_r_m2<Register8::A>, &CPU::LDH_an_r_m3<Register8::A> },
	/* E1 */ { &CPU::instructionNotImplemented },
	/* E2 */ { &CPU::LDH_ar_r_m1<Register8::C, Register8::A>, &CPU::LDH_ar_r_m2<Register8::C, Register8::A> },
	/* E3 */ { &CPU::instructionNotImplemented },
	/* E4 */ { &CPU::instructionNotImplemented },
	/* E5 */ { &CPU::instructionNotImplemented },
	/* E6 */ { &CPU::instructionNotImplemented },
	/* E7 */ { &CPU::instructionNotImplemented },
	/* E8 */ { &CPU::instructionNotImplemented },
	/* E9 */ { &CPU::instructionNotImplemented },
	/* EA */ { &CPU::LD_ann_r_m1<Register8::A>, &CPU::LD_ann_r_m2<Register8::A>, &CPU::LD_ann_r_m3<Register8::A>, &CPU::LD_ann_r_m4<Register8::A> },
	/* EB */ { &CPU::instructionNotImplemented },
	/* EC */ { &CPU::instructionNotImplemented },
	/* ED */ { &CPU::instructionNotImplemented },
	/* EE */ { &CPU::instructionNotImplemented },
	/* EF */ { &CPU::instructionNotImplemented },
	/* F0 */ { &CPU::LDH_r_an_m1<Register8::A>, &CPU::LDH_r_an_m2<Register8::A>, &CPU::LDH_r_an_m3<Register8::A> },
	/* F1 */ { &CPU::instructionNotImplemented },
	/* F2 */ { &CPU::LDH_r_ar_m1<Register8::A, Register8::C>, &CPU::LDH_r_ar_m2<Register8::A, Register8::C> },
	/* F3 */ { &CPU::instructionNotImplemented },
	/* F4 */ { &CPU::instructionNotImplemented },
	/* F5 */ { &CPU::instructionNotImplemented },
	/* F6 */ { &CPU::instructionNotImplemented },
	/* F7 */ { &CPU::instructionNotImplemented },
	/* F8 */ { &CPU::LD_rr_rrn_m1<Register16::HL, Register16::SP>, &CPU::LD_rr_rrn_m2<Register16::HL, Register16::SP>, &CPU::LD_rr_rrn_m3<Register16::HL, Register16::SP> },
	/* F9 */ { &CPU::LD_rr_rr_m1<Register16::SP, Register16::HL>, &CPU::LD_rr_rr_m2<Register16::SP, Register16::HL> },
	/* FA */ { &CPU::LD_r_ann_m1<Register8::A>, &CPU::LD_r_ann_m2<Register8::A>, &CPU::LD_r_ann_m3<Register8::A>, &CPU::LD_r_ann_m4<Register8::A> },
	/* FB */ { &CPU::instructionNotImplemented },
	/* FC */ { &CPU::instructionNotImplemented },
	/* FD */ { &CPU::instructionNotImplemented },
	/* FE */ { &CPU::instructionNotImplemented },
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
    DEBUG(1)
        << "------- CPU:tick -------\n"
        << status() << "\n"
        << std::endl;


    DEBUG(1) << ">> " << hex(currentInstruction.opcode) << " [MicroOp=" << std::to_string(currentInstruction.microop + 1) << "]" << std::endl;

    auto micro_op = instructions[currentInstruction.opcode][currentInstruction.microop++];
    if (!micro_op)
        throw IllegalInstructionException(
                "Illegal instruction: " + hex(currentInstruction.opcode) +
                " [MicroOp=" + std::to_string(currentInstruction.microop + 1) + "]");
    (this->*micro_op)();

    mCycles++;

    DEBUG(1)
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
    DEBUG(1) << "<fetch>" << std::endl;
    currentInstruction.opcode = bus.read(PC++);
    currentInstruction.microop = 0;
}

void CPU::instructionNotImplemented() {
    DEBUG(1) << "<not implemented>" << std::endl;
    throw InstructionNotImplementedException(
            "Instruction not implemented: " + hex(currentInstruction.opcode) +
            " [MicroOp=" + std::to_string(currentInstruction.microop + 1) + "]");
}

void CPU::NOP_m1() {
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

// TODO: when are flags set?

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrn_m1() {
    s1 = (int8_t) bus.read(PC++);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrn_m2() {
    int64_t result = readRegister16<rr2>() + u1;
    writeFlag<Flag::C>(result < 0 || result > 0xFFFF);
    writeRegister16<rr1>(result);
    // TODO: half carry...
    // TODO: does it work?
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrn_m3() {
    fetch();
}

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

template<CPU::Register8 r>
void CPU::XOR_r_m1() {
    uint8_t result = readRegister8<Register8::A>() ^ readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result);
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
    writeRegister8<Register8::A>(u1);
    writeFlag<Flag::Z>(u1);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

