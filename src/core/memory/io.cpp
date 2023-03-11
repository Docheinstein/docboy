#include "io.h"
#include "core/definitions.h"

IO::IO() : Impl::Memory(MemoryMap::IO::SIZE) {

}

uint8_t IO::read(uint16_t index) const {
    uint16_t reg = MemoryMap::IO::START + index;
    switch (reg) {
//    case Registers::Joypad::P1:
//        return 0xFF;
    default:
        return Impl::Memory::read(index);
    }
}

void IO::write(uint16_t index, uint8_t value) {
    uint16_t reg = MemoryMap::IO::START + index;
    switch (reg) {
    case Registers::Timers::DIV:
        Impl::Memory::write(index, 0);
        break;
    default:
        Impl::Memory::write(index, value);
    }
}

uint8_t IO::readP1() const {
    return read(Registers::Joypad::P1 - MemoryMap::IO::START);
}

void IO::writeP1(uint8_t value) {
    write(Registers::Joypad::P1 - MemoryMap::IO::START, value);
}

uint8_t IO::readSB() const {
    return read(Registers::Serial::SB - MemoryMap::IO::START);
}

void IO::writeSB(uint8_t value) {
    write(Registers::Serial::SB - MemoryMap::IO::START, value);
}

uint8_t IO::readSC() const {
    return read(Registers::Serial::SC - MemoryMap::IO::START);
}

void IO::writeSC(uint8_t value) {
    write(Registers::Serial::SC - MemoryMap::IO::START, value);
}

uint8_t IO::readDIV() const {
    return read(Registers::Timers::DIV - MemoryMap::IO::START);
}

void IO::writeDIV(uint8_t value) {
    write(Registers::Timers::DIV - MemoryMap::IO::START, value);
}

uint8_t IO::readTIMA() const {
    return read(Registers::Timers::TIMA - MemoryMap::IO::START);
}

void IO::writeTIMA(uint8_t value) {
    write(Registers::Timers::TIMA - MemoryMap::IO::START, value);
}

uint8_t IO::readTMA() const {
    return read(Registers::Timers::TMA - MemoryMap::IO::START);
}

void IO::writeTMA(uint8_t value) {
    write(Registers::Timers::TMA - MemoryMap::IO::START, value);
}

uint8_t IO::readTAC() const {
    return read(Registers::Timers::TAC - MemoryMap::IO::START);
}

void IO::writeTAC(uint8_t value) {
    write(Registers::Timers::TAC - MemoryMap::IO::START, value);
}

uint8_t IO::readIF() const {
    return read(Registers::Interrupts::IF - MemoryMap::IO::START);
}

void IO::writeIF(uint8_t value) {
    write(Registers::Interrupts::IF - MemoryMap::IO::START, value);
}

uint8_t IO::readNR10() const {
    return read(Registers::Sound::NR10 - MemoryMap::IO::START);
}

void IO::writeNR10(uint8_t value) {
    write(Registers::Sound::NR10 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR11() const {
    return read(Registers::Sound::NR11 - MemoryMap::IO::START);
}

void IO::writeNR11(uint8_t value) {
    write(Registers::Sound::NR11 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR12() const {
    return read(Registers::Sound::NR12 - MemoryMap::IO::START);
}

void IO::writeNR12(uint8_t value) {
    write(Registers::Sound::NR12 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR13() const {
    return read(Registers::Sound::NR13 - MemoryMap::IO::START);
}

void IO::writeNR13(uint8_t value) {
    write(Registers::Sound::NR13 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR14() const {
    return read(Registers::Sound::NR14 - MemoryMap::IO::START);
}

void IO::writeNR14(uint8_t value) {
    write(Registers::Sound::NR14 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR21() const {
    return read(Registers::Sound::NR21 - MemoryMap::IO::START);
}

void IO::writeNR21(uint8_t value) {
    write(Registers::Sound::NR21 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR22() const {
    return read(Registers::Sound::NR22 - MemoryMap::IO::START);
}

void IO::writeNR22(uint8_t value) {
    write(Registers::Sound::NR22 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR23() const {
    return read(Registers::Sound::NR23 - MemoryMap::IO::START);
}

void IO::writeNR23(uint8_t value) {
    write(Registers::Sound::NR23 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR24() const {
    return read(Registers::Sound::NR24 - MemoryMap::IO::START);
}

void IO::writeNR24(uint8_t value) {
    write(Registers::Sound::NR24 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR30() const {
    return read(Registers::Sound::NR30 - MemoryMap::IO::START);
}

void IO::writeNR30(uint8_t value) {
    write(Registers::Sound::NR30 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR31() const {
    return read(Registers::Sound::NR31 - MemoryMap::IO::START);
}

void IO::writeNR31(uint8_t value) {
    write(Registers::Sound::NR31 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR32() const {
    return read(Registers::Sound::NR32 - MemoryMap::IO::START);
}

void IO::writeNR32(uint8_t value) {
    write(Registers::Sound::NR32 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR33() const {
    return read(Registers::Sound::NR33 - MemoryMap::IO::START);
}

void IO::writeNR33(uint8_t value) {
    write(Registers::Sound::NR33 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR34() const {
    return read(Registers::Sound::NR34 - MemoryMap::IO::START);
}

void IO::writeNR34(uint8_t value) {
    write(Registers::Sound::NR34 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR41() const {
    return read(Registers::Sound::NR41 - MemoryMap::IO::START);
}

void IO::writeNR41(uint8_t value) {
    write(Registers::Sound::NR41 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR42() const {
    return read(Registers::Sound::NR42 - MemoryMap::IO::START);
}

void IO::writeNR42(uint8_t value) {
    write(Registers::Sound::NR42 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR43() const {
    return read(Registers::Sound::NR43 - MemoryMap::IO::START);
}

void IO::writeNR43(uint8_t value) {
    write(Registers::Sound::NR43 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR44() const {
    return read(Registers::Sound::NR44 - MemoryMap::IO::START);
}

void IO::writeNR44(uint8_t value) {
    write(Registers::Sound::NR44 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR50() const {
    return read(Registers::Sound::NR50 - MemoryMap::IO::START);
}

void IO::writeNR50(uint8_t value) {
    write(Registers::Sound::NR50 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR51() const {
    return read(Registers::Sound::NR51 - MemoryMap::IO::START);
}

void IO::writeNR51(uint8_t value) {
    write(Registers::Sound::NR51 - MemoryMap::IO::START, value);
}

uint8_t IO::readNR52() const {
    return read(Registers::Sound::NR52 - MemoryMap::IO::START);
}

void IO::writeNR52(uint8_t value) {
    write(Registers::Sound::NR52 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE0() const {
    return read(Registers::Sound::WAVE0 - MemoryMap::IO::START);
}

void IO::writeWAVE0(uint8_t value) {
    write(Registers::Sound::WAVE0 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE1() const {
    return read(Registers::Sound::WAVE1 - MemoryMap::IO::START);
}

void IO::writeWAVE1(uint8_t value) {
    write(Registers::Sound::WAVE1 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE2() const {
    return read(Registers::Sound::WAVE2 - MemoryMap::IO::START);
}

void IO::writeWAVE2(uint8_t value) {
    write(Registers::Sound::WAVE2 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE3() const {
    return read(Registers::Sound::WAVE3 - MemoryMap::IO::START);
}

void IO::writeWAVE3(uint8_t value) {
    write(Registers::Sound::WAVE3 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE4() const {
    return read(Registers::Sound::WAVE4 - MemoryMap::IO::START);
}

void IO::writeWAVE4(uint8_t value) {
    write(Registers::Sound::WAVE4 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE5() const {
    return read(Registers::Sound::WAVE5 - MemoryMap::IO::START);
}

void IO::writeWAVE5(uint8_t value) {
    write(Registers::Sound::WAVE5 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE6() const {
    return read(Registers::Sound::WAVE6 - MemoryMap::IO::START);
}

void IO::writeWAVE6(uint8_t value) {
    write(Registers::Sound::WAVE6 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE7() const {
    return read(Registers::Sound::WAVE7 - MemoryMap::IO::START);
}

void IO::writeWAVE7(uint8_t value) {
    write(Registers::Sound::WAVE7 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE8() const {
    return read(Registers::Sound::WAVE8 - MemoryMap::IO::START);
}

void IO::writeWAVE8(uint8_t value) {
    write(Registers::Sound::WAVE8 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVE9() const {
    return read(Registers::Sound::WAVE9 - MemoryMap::IO::START);
}

void IO::writeWAVE9(uint8_t value) {
    write(Registers::Sound::WAVE9 - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVEA() const {
    return read(Registers::Sound::WAVEA - MemoryMap::IO::START);
}

void IO::writeWAVEA(uint8_t value) {
    write(Registers::Sound::WAVEA - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVEB() const {
    return read(Registers::Sound::WAVEB - MemoryMap::IO::START);
}

void IO::writeWAVEB(uint8_t value) {
    write(Registers::Sound::WAVEB - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVEC() const {
    return read(Registers::Sound::WAVEC - MemoryMap::IO::START);
}

void IO::writeWAVEC(uint8_t value) {
    write(Registers::Sound::WAVEC - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVED() const {
    return read(Registers::Sound::WAVED - MemoryMap::IO::START);
}

void IO::writeWAVED(uint8_t value) {
    write(Registers::Sound::WAVED - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVEE() const {
    return read(Registers::Sound::WAVEE - MemoryMap::IO::START);
}

void IO::writeWAVEE(uint8_t value) {
    write(Registers::Sound::WAVEE - MemoryMap::IO::START, value);
}

uint8_t IO::readWAVEF() const {
    return read(Registers::Sound::WAVEF - MemoryMap::IO::START);
}

void IO::writeWAVEF(uint8_t value) {
    write(Registers::Sound::WAVEF - MemoryMap::IO::START, value);
}

uint8_t IO::readLCDC() const {
    return read(Registers::LCD::LCDC - MemoryMap::IO::START);
}

void IO::writeLCDC(uint8_t value) {
    write(Registers::LCD::LCDC - MemoryMap::IO::START, value);
}

uint8_t IO::readSTAT() const {
    return read(Registers::LCD::STAT - MemoryMap::IO::START);
}

void IO::writeSTAT(uint8_t value) {
    write(Registers::LCD::STAT - MemoryMap::IO::START, value);
}

uint8_t IO::readSCY() const {
    return read(Registers::LCD::SCY - MemoryMap::IO::START);
}

void IO::writeSCY(uint8_t value) {
    write(Registers::LCD::SCY - MemoryMap::IO::START, value);
}

uint8_t IO::readSCX() const {
    return read(Registers::LCD::SCX - MemoryMap::IO::START);
}

void IO::writeSCX(uint8_t value) {
    write(Registers::LCD::SCX - MemoryMap::IO::START, value);
}

uint8_t IO::readLY() const {
    return read(Registers::LCD::LY - MemoryMap::IO::START);
}

void IO::writeLY(uint8_t value) {
    write(Registers::LCD::LY - MemoryMap::IO::START, value);
}

uint8_t IO::readLYC() const {
    return read(Registers::LCD::LYC - MemoryMap::IO::START);
}

void IO::writeLYC(uint8_t value) {
    write(Registers::LCD::LYC - MemoryMap::IO::START, value);
}

uint8_t IO::readDMA() const {
    return read(Registers::LCD::DMA - MemoryMap::IO::START);
}

void IO::writeDMA(uint8_t value) {
    write(Registers::LCD::DMA - MemoryMap::IO::START, value);
}

uint8_t IO::readBGP() const {
    return read(Registers::LCD::BGP - MemoryMap::IO::START);
}

void IO::writeBGP(uint8_t value) {
    write(Registers::LCD::BGP - MemoryMap::IO::START, value);
}

uint8_t IO::readOBP0() const {
    return read(Registers::LCD::OBP0 - MemoryMap::IO::START);
}

void IO::writeOBP0(uint8_t value) {
    write(Registers::LCD::OBP0 - MemoryMap::IO::START, value);
}

uint8_t IO::readOBP1() const {
    return read(Registers::LCD::OBP1 - MemoryMap::IO::START);
}

void IO::writeOBP1(uint8_t value) {
    write(Registers::LCD::OBP1 - MemoryMap::IO::START, value);
}

uint8_t IO::readWY() const {
    return read(Registers::LCD::WY - MemoryMap::IO::START);
}

void IO::writeWY(uint8_t value) {
    write(Registers::LCD::WY - MemoryMap::IO::START, value);
}

uint8_t IO::readWX() const {
    return read(Registers::LCD::WX - MemoryMap::IO::START);
}

void IO::writeWX(uint8_t value) {
    write(Registers::LCD::WX - MemoryMap::IO::START, value);
}

uint8_t IO::readBOOTROM() const {
    return read(Registers::Boot::BOOTROM - MemoryMap::IO::START);
}

void IO::writeBOOTROM(uint8_t value) {
    write(Registers::Boot::BOOTROM - MemoryMap::IO::START, value);
}


