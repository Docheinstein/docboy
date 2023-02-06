#include "cpu.h"
#include "log/log.h"
#include "utils/binutils.h"
#include <functional>

//static const uint8_t INSTRUCTIONS_M_CYCLES[256] = {
//    1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1,
//    1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,
//    2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1,
//    2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1,
//    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
//    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
//    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
//    2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
//    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
//    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
//    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
//    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
//    2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 0, 3, 6, 2, 4,
//    2, 3, 3, 0, 3, 4, 2, 4, 2, 4, 3, 0, 3, 0, 2, 4,
//    3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4,
//    3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4
//};

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
    instructions {
	/* 00 */ { &CPU::NOP_m1 },
	/* 01 */ { &CPU::instructionNotImplemented },
	/* 02 */ { &CPU::instructionNotImplemented },
	/* 03 */ { &CPU::instructionNotImplemented },
	/* 04 */ { &CPU::instructionNotImplemented },
	/* 05 */ { &CPU::instructionNotImplemented },
	/* 06 */ { &CPU::instructionNotImplemented },
	/* 07 */ { &CPU::instructionNotImplemented },
	/* 08 */ { &CPU::instructionNotImplemented },
	/* 09 */ { &CPU::instructionNotImplemented },
	/* 0A */ { &CPU::instructionNotImplemented },
	/* 0B */ { &CPU::instructionNotImplemented },
	/* 0C */ { &CPU::instructionNotImplemented },
	/* 0D */ { &CPU::instructionNotImplemented },
	/* 0E */ { &CPU::instructionNotImplemented },
	/* 0F */ { &CPU::instructionNotImplemented },
	/* 10 */ { &CPU::instructionNotImplemented },
	/* 11 */ { &CPU::instructionNotImplemented },
	/* 12 */ { &CPU::instructionNotImplemented },
	/* 13 */ { &CPU::instructionNotImplemented },
	/* 14 */ { &CPU::instructionNotImplemented },
	/* 15 */ { &CPU::instructionNotImplemented },
	/* 16 */ { &CPU::instructionNotImplemented },
	/* 17 */ { &CPU::instructionNotImplemented },
	/* 18 */ { &CPU::instructionNotImplemented },
	/* 19 */ { &CPU::instructionNotImplemented },
	/* 1A */ { &CPU::instructionNotImplemented },
	/* 1B */ { &CPU::instructionNotImplemented },
	/* 1C */ { &CPU::instructionNotImplemented },
	/* 1D */ { &CPU::instructionNotImplemented },
	/* 1E */ { &CPU::instructionNotImplemented },
	/* 1F */ { &CPU::instructionNotImplemented },
	/* 20 */ { &CPU::instructionNotImplemented },
	/* 21 */ { &CPU::instructionNotImplemented },
	/* 22 */ { &CPU::instructionNotImplemented },
	/* 23 */ { &CPU::instructionNotImplemented },
	/* 24 */ { &CPU::instructionNotImplemented },
	/* 25 */ { &CPU::instructionNotImplemented },
	/* 26 */ { &CPU::instructionNotImplemented },
	/* 27 */ { &CPU::instructionNotImplemented },
	/* 28 */ { &CPU::instructionNotImplemented },
	/* 29 */ { &CPU::instructionNotImplemented },
	/* 2A */ { &CPU::instructionNotImplemented },
	/* 2B */ { &CPU::instructionNotImplemented },
	/* 2C */ { &CPU::instructionNotImplemented },
	/* 2D */ { &CPU::instructionNotImplemented },
	/* 2E */ { &CPU::instructionNotImplemented },
	/* 2F */ { &CPU::instructionNotImplemented },
	/* 30 */ { &CPU::instructionNotImplemented },
	/* 31 */ { &CPU::instructionNotImplemented },
	/* 32 */ { &CPU::instructionNotImplemented },
	/* 33 */ { &CPU::instructionNotImplemented },
	/* 34 */ { &CPU::instructionNotImplemented },
	/* 35 */ { &CPU::instructionNotImplemented },
	/* 36 */ { &CPU::instructionNotImplemented },
	/* 37 */ { &CPU::instructionNotImplemented },
	/* 38 */ { &CPU::instructionNotImplemented },
	/* 39 */ { &CPU::instructionNotImplemented },
	/* 3A */ { &CPU::instructionNotImplemented },
	/* 3B */ { &CPU::instructionNotImplemented },
	/* 3C */ { &CPU::instructionNotImplemented },
	/* 3D */ { &CPU::instructionNotImplemented },
	/* 3E */ { &CPU::instructionNotImplemented },
	/* 3F */ { &CPU::instructionNotImplemented },
	/* 40 */ { &CPU::instructionNotImplemented },
	/* 41 */ { &CPU::instructionNotImplemented },
	/* 42 */ { &CPU::instructionNotImplemented },
	/* 43 */ { &CPU::instructionNotImplemented },
	/* 44 */ { &CPU::instructionNotImplemented },
	/* 45 */ { &CPU::instructionNotImplemented },
	/* 46 */ { &CPU::instructionNotImplemented },
	/* 47 */ { &CPU::instructionNotImplemented },
	/* 48 */ { &CPU::instructionNotImplemented },
	/* 49 */ { &CPU::instructionNotImplemented },
	/* 4A */ { &CPU::instructionNotImplemented },
	/* 4B */ { &CPU::instructionNotImplemented },
	/* 4C */ { &CPU::instructionNotImplemented },
	/* 4D */ { &CPU::instructionNotImplemented },
	/* 4E */ { &CPU::instructionNotImplemented },
	/* 4F */ { &CPU::instructionNotImplemented },
	/* 50 */ { &CPU::instructionNotImplemented },
	/* 51 */ { &CPU::instructionNotImplemented },
	/* 52 */ { &CPU::instructionNotImplemented },
	/* 53 */ { &CPU::instructionNotImplemented },
	/* 54 */ { &CPU::instructionNotImplemented },
	/* 55 */ { &CPU::instructionNotImplemented },
	/* 56 */ { &CPU::instructionNotImplemented },
	/* 57 */ { &CPU::instructionNotImplemented },
	/* 58 */ { &CPU::instructionNotImplemented },
	/* 59 */ { &CPU::instructionNotImplemented },
	/* 5A */ { &CPU::instructionNotImplemented },
	/* 5B */ { &CPU::instructionNotImplemented },
	/* 5C */ { &CPU::instructionNotImplemented },
	/* 5D */ { &CPU::instructionNotImplemented },
	/* 5E */ { &CPU::instructionNotImplemented },
	/* 5F */ { &CPU::instructionNotImplemented },
	/* 60 */ { &CPU::instructionNotImplemented },
	/* 61 */ { &CPU::instructionNotImplemented },
	/* 62 */ { &CPU::instructionNotImplemented },
	/* 63 */ { &CPU::instructionNotImplemented },
	/* 64 */ { &CPU::instructionNotImplemented },
	/* 65 */ { &CPU::instructionNotImplemented },
	/* 66 */ { &CPU::instructionNotImplemented },
	/* 67 */ { &CPU::instructionNotImplemented },
	/* 68 */ { &CPU::instructionNotImplemented },
	/* 69 */ { &CPU::instructionNotImplemented },
	/* 6A */ { &CPU::instructionNotImplemented },
	/* 6B */ { &CPU::instructionNotImplemented },
	/* 6C */ { &CPU::instructionNotImplemented },
	/* 6D */ { &CPU::instructionNotImplemented },
	/* 6E */ { &CPU::instructionNotImplemented },
	/* 6F */ { &CPU::instructionNotImplemented },
	/* 70 */ { &CPU::instructionNotImplemented },
	/* 71 */ { &CPU::instructionNotImplemented },
	/* 72 */ { &CPU::instructionNotImplemented },
	/* 73 */ { &CPU::instructionNotImplemented },
	/* 74 */ { &CPU::instructionNotImplemented },
	/* 75 */ { &CPU::instructionNotImplemented },
	/* 76 */ { &CPU::instructionNotImplemented },
	/* 77 */ { &CPU::instructionNotImplemented },
	/* 78 */ { &CPU::instructionNotImplemented },
	/* 79 */ { &CPU::instructionNotImplemented },
	/* 7A */ { &CPU::instructionNotImplemented },
	/* 7B */ { &CPU::instructionNotImplemented },
	/* 7C */ { &CPU::instructionNotImplemented },
	/* 7D */ { &CPU::instructionNotImplemented },
	/* 7E */ { &CPU::instructionNotImplemented },
	/* 7F */ { &CPU::instructionNotImplemented },
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
	/* A8 */ { &CPU::XORr_m1<REG_B>, &CPU::XORr_m2<REG_B> },
	/* A9 */ { &CPU::XORr_m1<REG_C>, &CPU::XORr_m2<REG_C> },
	/* AA */ { &CPU::XORr_m1<REG_D>, &CPU::XORr_m2<REG_D> },
	/* AB */ { &CPU::XORr_m1<REG_E>, &CPU::XORr_m2<REG_E> },
	/* AC */ { &CPU::XORr_m1<REG_H>, &CPU::XORr_m2<REG_H> },
	/* AD */ { &CPU::XORr_m1<REG_L>, &CPU::XORr_m2<REG_L> },
	/* AE */ { &CPU::XORa_m1<REG_HL>, &CPU::XORa_m2<REG_HL> },
	/* AF */ { &CPU::XORr_m1<REG_A>, &CPU::XORr_m2<REG_A> },
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
	/* C3 */ { &CPU::JP_a16_m1, &CPU::JP_a16_m2, &CPU::JP_a16_m3, &CPU::JP_a16_m4 },
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
	/* E0 */ { &CPU::instructionNotImplemented },
	/* E1 */ { &CPU::instructionNotImplemented },
	/* E2 */ { &CPU::instructionNotImplemented },
	/* E3 */ { &CPU::instructionNotImplemented },
	/* E4 */ { &CPU::instructionNotImplemented },
	/* E5 */ { &CPU::instructionNotImplemented },
	/* E6 */ { &CPU::instructionNotImplemented },
	/* E7 */ { &CPU::instructionNotImplemented },
	/* E8 */ { &CPU::instructionNotImplemented },
	/* E9 */ { &CPU::instructionNotImplemented },
	/* EA */ { &CPU::instructionNotImplemented },
	/* EB */ { &CPU::instructionNotImplemented },
	/* EC */ { &CPU::instructionNotImplemented },
	/* ED */ { &CPU::instructionNotImplemented },
	/* EE */ { &CPU::instructionNotImplemented },
	/* EF */ { &CPU::instructionNotImplemented },
	/* F0 */ { &CPU::instructionNotImplemented },
	/* F1 */ { &CPU::instructionNotImplemented },
	/* F2 */ { &CPU::instructionNotImplemented },
	/* F3 */ { &CPU::instructionNotImplemented },
	/* F4 */ { &CPU::instructionNotImplemented },
	/* F5 */ { &CPU::instructionNotImplemented },
	/* F6 */ { &CPU::instructionNotImplemented },
	/* F7 */ { &CPU::instructionNotImplemented },
	/* F8 */ { &CPU::instructionNotImplemented },
	/* F9 */ { &CPU::instructionNotImplemented },
	/* FA */ { &CPU::instructionNotImplemented },
	/* FB */ { &CPU::instructionNotImplemented },
	/* FC */ { &CPU::instructionNotImplemented },
	/* FD */ { &CPU::instructionNotImplemented },
	/* FE */ { &CPU::instructionNotImplemented },
	/* FF */ { &CPU::instructionNotImplemented },
    } {

}

void CPU::tick() {
    DEBUG(1)
        << "------- CPU:tick -------\n"
        << status() << "\n"
        << std::endl;


    DEBUG(1) << ">> " << hex(currentInstruction.instruction) << " [MicroOp=" << std::to_string(currentInstruction.m + 1) << "]" << std::endl;

    auto micro_op = instructions[currentInstruction.instruction][currentInstruction.m++];
    (this->*micro_op)();

    mCycles++;

    DEBUG(1)
        << "\n"
        << status() << "\n"
        << "---------------------\n" << std::endl;
}

std::string CPU::status() const {
    std::stringstream ss;
    ss  << "PC   "
        << "SP   "
        << "AF               "
        << "BC               "
        << "DE               "
        << "HL"
        << "\n"
        << hex(PC) << " "
        << hex(SP) << " "
        << bin(AF) << " "
        << bin(BC) << " "
        << bin(DE) << " "
        << bin(HL)
        << "\n"
        << "instruction  "
        << "M-cycles"
        << "\n"
        << hex(currentInstruction.instruction) << "[M=" << std::to_string(currentInstruction.m + 1) << "]      "
        << mCycles
    ;
    return ss.str();
}

void CPU::fetch() {
    DEBUG(1) << "<fetch>" << std::endl;
    currentInstruction.instruction = bus.read(PC++);
    currentInstruction.m = 0;
}

void CPU::instructionNotImplemented() {
    throw std::runtime_error("Instruction not implemented: " +
        hex(currentInstruction.instruction) + " [MicroOp=" + std::to_string(currentInstruction.m + 1) + "]");
}

template<>
uint8_t CPU::readRegister8<CPU::REG_A>() const {
    return get_byte<1>(AF);
}

template<>
uint8_t CPU::readRegister8<CPU::REG_B>() const {
    return get_byte<1>(BC);
}

template<>
uint8_t CPU::readRegister8<CPU::REG_C>() const {
    return get_byte<0>(BC);
}

template<>
uint8_t CPU::readRegister8<CPU::REG_D>() const {
    return get_byte<1>(DE);
}

template<>
uint8_t CPU::readRegister8<CPU::REG_E>() const {
    return get_byte<0>(DE);
}

template<>
uint8_t CPU::readRegister8<CPU::REG_F>() const {
    return get_byte<0>(AF);
}

template<>
uint8_t CPU::readRegister8<CPU::REG_H>() const {
    return get_byte<1>(HL);
}

template<>
uint8_t CPU::readRegister8<CPU::REG_L>() const {
    return get_byte<0>(HL);
}

template<>
void CPU::writeRegister8<CPU::REG_A>(uint8_t value) {
    return set_byte<1>(AF, value);
}

template<>
void CPU::writeRegister8<CPU::REG_B>(uint8_t value) {
    return set_byte<1>(BC, value);
}

template<>
void CPU::writeRegister8<CPU::REG_C>(uint8_t value) {
    return set_byte<0>(BC, value);
}

template<>
void CPU::writeRegister8<CPU::REG_D>(uint8_t value) {
    return set_byte<1>(DE, value);
}

template<>
void CPU::writeRegister8<CPU::REG_E>(uint8_t value) {
    return set_byte<0>(DE, value);
}

template<>
void CPU::writeRegister8<CPU::REG_F>(uint8_t value) {
    return set_byte<0>(AF, value);
}

template<>
void CPU::writeRegister8<CPU::REG_H>(uint8_t value) {
    return set_byte<1>(HL, value);
}

template<>
void CPU::writeRegister8<CPU::REG_L>(uint8_t value) {
    return set_byte<0>(HL, value);
}

template<>
uint16_t CPU::readRegister16<CPU::REG_AF>() const {
    return AF;
}

template<>
uint16_t CPU::readRegister16<CPU::REG_BC>() const {
    return BC;
}

template<>
uint16_t CPU::readRegister16<CPU::REG_DE>() const {
    return DE;
}
template<>
uint16_t CPU::readRegister16<CPU::REG_HL>() const {
    return HL;
}

template<>
void CPU::writeRegister16<CPU::REG_AF>(uint16_t value) {
    AF = value;
}

template<>
void CPU::writeRegister16<CPU::REG_BC>(uint16_t value) {
    BC = value;
}

template<>
void CPU::writeRegister16<CPU::REG_DE>(uint16_t value) {
    DE = value;
}

template<>
void CPU::writeRegister16<CPU::REG_HL>(uint16_t value) {
    HL = value;
}

template<CPU::Flag flag>
[[nodiscard]] bool CPU::readFlag() const {
    return (AF >> flag) & 1;
}

template<CPU::Flag flag>
void CPU::writeFlag(bool value) {
    AF &= (~((uint8_t) 1 << flag));
    AF |= ((value ? 1 : 0) << flag);
}

void CPU::NOP_m1() {
    fetch();
}

void  CPU::JP_a16_m1() {
    tmp1 = bus.read(PC++);
}

void CPU::JP_a16_m2() {
    tmp2 = bus.read(PC++);
}

void CPU::JP_a16_m3() {
    PC = tmp2 << 8 | tmp1;
}

void CPU::JP_a16_m4() {
    fetch();
}

template<CPU::Register8 reg>
void CPU::XORr_m1() {
    uint8_t result = readRegister8<REG_A>() ^ readRegister8<reg>();
    writeRegister8<REG_A>(result);
    writeFlag<FLAG_Z>(result);
    writeFlag<FLAG_N>(false);
    writeFlag<FLAG_H>(false);
    writeFlag<FLAG_C>(false);
}

template<CPU::Register8 reg>
void CPU::XORr_m2() {
    fetch();
}

template<CPU::Register16 reg>
void CPU::XORa_m1() {
    tmp1 = bus.read(readRegister16<reg>());
}

template<CPU::Register16 reg>
void CPU::XORa_m2() {
    writeRegister8<REG_A>(tmp1);
    writeFlag<FLAG_Z>(tmp1);
    writeFlag<FLAG_N>(false);
    writeFlag<FLAG_H>(false);
    writeFlag<FLAG_C>(false);
}

template<CPU::Register16 reg>
void CPU::XORa_m3() {
    fetch();
}

