#include "bus.h"
#include "core/cartridge/cartridge.h"
#include "core/io/boot.h"
#include "core/io/interrupts.h"
#include "core/io/joypad.h"
#include "core/io/lcd.h"
#include "core/io/serial.h"
#include "core/io/sound.h"
#include "core/io/timers.h"
#include "core/memory/memory.h"
#include "utils/log.h"

Bus::Bus(IMemory& vram, IMemory& wram1, IMemory& wram2, IMemory& oam, IMemory& hram, ICartridge& cartridge,
         IBootROM& bootRom, IJoypadIO& joypad, ISerialIO& serial, ITimersIO& timers, IInterruptsIO& interrupts,
         ISoundIO& sound, ILCDIO& lcd, IBootIO& boot) :
    vram(vram),
    wram1(wram1),
    wram2(wram2),
    oam(oam),
    hram(hram),
    cartridge(cartridge),
    bootRom(bootRom),
    joypad(joypad),
    serial(serial),
    timers(timers),
    interrupts(interrupts),
    sound(sound),
    lcd(lcd),
    boot(boot),
    io{
        /* FF00 */ {.read = &Bus::readP1, .write = &Bus::writeP1},
        /* FF01 */ {.read = &Bus::readSB, .write = &Bus::writeSB},
        /* FF02 */ {.read = &Bus::readSC, .write = &Bus::writeSC},
        /* FF03 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF04 */ {.read = &Bus::readDIV, .write = &Bus::writeDIV},
        /* FF05 */ {.read = &Bus::readTIMA, .write = &Bus::writeTIMA},
        /* FF06 */ {.read = &Bus::readTMA, .write = &Bus::writeTMA},
        /* FF07 */ {.read = &Bus::readTAC, .write = &Bus::writeTAC},
        /* FF08 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF09 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF0A */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF0B */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF0C */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF0D */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF0E */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF0F */ {.read = &Bus::readIF, .write = &Bus::writeIF},
        /* FF10 */ {.read = &Bus::readNR10, .write = &Bus::writeNR10},
        /* FF11 */ {.read = &Bus::readNR11, .write = &Bus::writeNR11},
        /* FF12 */ {.read = &Bus::readNR12, .write = &Bus::writeNR12},
        /* FF13 */ {.read = &Bus::readNR13, .write = &Bus::writeNR13},
        /* FF14 */ {.read = &Bus::readNR14, .write = &Bus::writeNR14},
        /* FF15 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF16 */ {.read = &Bus::readNR21, .write = &Bus::writeNR21},
        /* FF17 */ {.read = &Bus::readNR22, .write = &Bus::writeNR22},
        /* FF18 */ {.read = &Bus::readNR23, .write = &Bus::writeNR23},
        /* FF19 */ {.read = &Bus::readNR24, .write = &Bus::writeNR24},
        /* FF1A */ {.read = &Bus::readNR30, .write = &Bus::writeNR30},
        /* FF1B */ {.read = &Bus::readNR31, .write = &Bus::writeNR31},
        /* FF1C */ {.read = &Bus::readNR32, .write = &Bus::writeNR32},
        /* FF1D */ {.read = &Bus::readNR33, .write = &Bus::writeNR33},
        /* FF1E */ {.read = &Bus::readNR34, .write = &Bus::writeNR34},
        /* FF1F */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF20 */ {.read = &Bus::readNR41, .write = &Bus::writeNR41},
        /* FF21 */ {.read = &Bus::readNR42, .write = &Bus::writeNR42},
        /* FF22 */ {.read = &Bus::readNR43, .write = &Bus::writeNR43},
        /* FF23 */ {.read = &Bus::readNR44, .write = &Bus::writeNR44},
        /* FF24 */ {.read = &Bus::readNR50, .write = &Bus::writeNR50},
        /* FF25 */ {.read = &Bus::readNR51, .write = &Bus::writeNR51},
        /* FF26 */ {.read = &Bus::readNR52, .write = &Bus::writeNR52},
        /* FF27 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF28 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF29 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF2A */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF2B */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF2C */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF2D */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF2E */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF2F */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF30 */ {.read = &Bus::readWAVE0, .write = &Bus::writeWAVE0},
        /* FF31 */ {.read = &Bus::readWAVE1, .write = &Bus::writeWAVE1},
        /* FF32 */ {.read = &Bus::readWAVE2, .write = &Bus::writeWAVE2},
        /* FF33 */ {.read = &Bus::readWAVE3, .write = &Bus::writeWAVE3},
        /* FF34 */ {.read = &Bus::readWAVE4, .write = &Bus::writeWAVE4},
        /* FF35 */ {.read = &Bus::readWAVE5, .write = &Bus::writeWAVE5},
        /* FF36 */ {.read = &Bus::readWAVE6, .write = &Bus::writeWAVE6},
        /* FF37 */ {.read = &Bus::readWAVE7, .write = &Bus::writeWAVE7},
        /* FF38 */ {.read = &Bus::readWAVE8, .write = &Bus::writeWAVE8},
        /* FF39 */ {.read = &Bus::readWAVE9, .write = &Bus::writeWAVE9},
        /* FF3A */ {.read = &Bus::readWAVEA, .write = &Bus::writeWAVEA},
        /* FF3B */ {.read = &Bus::readWAVEB, .write = &Bus::writeWAVEB},
        /* FF3C */ {.read = &Bus::readWAVEC, .write = &Bus::writeWAVEC},
        /* FF3D */ {.read = &Bus::readWAVED, .write = &Bus::writeWAVED},
        /* FF3E */ {.read = &Bus::readWAVEE, .write = &Bus::writeWAVEE},
        /* FF3F */ {.read = &Bus::readWAVEF, .write = &Bus::writeWAVEF},
        /* FF40 */ {.read = &Bus::readLCDC, .write = &Bus::writeLCDC},
        /* FF41 */ {.read = &Bus::readSTAT, .write = &Bus::writeSTAT},
        /* FF42 */ {.read = &Bus::readSCY, .write = &Bus::writeSCY},
        /* FF43 */ {.read = &Bus::readSCX, .write = &Bus::writeSCX},
        /* FF44 */ {.read = &Bus::readLY, .write = &Bus::writeLY},
        /* FF45 */ {.read = &Bus::readLYC, .write = &Bus::writeLYC},
        /* FF46 */ {.read = &Bus::readDMA, .write = &Bus::writeDMA},
        /* FF47 */ {.read = &Bus::readBGP, .write = &Bus::writeBGP},
        /* FF48 */ {.read = &Bus::readOBP0, .write = &Bus::writeOBP0},
        /* FF49 */ {.read = &Bus::readOBP1, .write = &Bus::writeOBP1},
        /* FF4A */ {.read = &Bus::readWY, .write = &Bus::writeWY},
        /* FF4B */ {.read = &Bus::readWX, .write = &Bus::writeWX},
        /* FF4C */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF4D */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF4E */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF4F */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF50 */ {.read = &Bus::readBOOT, .write = &Bus::writeBOOT},
        /* FF51 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF52 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF53 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF54 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF55 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF56 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF57 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF58 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF59 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF5A */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF5B */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF5C */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF5D */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF5E */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF5F */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF60 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF61 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF62 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF63 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF64 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF65 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF66 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF67 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF68 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF69 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF6A */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF6B */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF6C */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF6D */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF6E */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF6F */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF70 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF71 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF72 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF73 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF74 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF75 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF76 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF77 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF78 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF79 */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF7A */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF7B */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF7C */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF7D */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF7E */ {.read = &Bus::readNull, .write = &Bus::writeNull},
        /* FF7F */ {.read = &Bus::readNull, .write = &Bus::writeNull},
    } {
}

uint8_t Bus::read(uint16_t addr) const {
    if (addr <= MemoryMap::ROM1::END) {
        if (boot.readBOOT() == 0 && addr < 0x100 /* TODO: better way to check than < 0x100? */)
            return bootRom.read(addr);
        return cartridge.read(addr);
    } else if (addr <= MemoryMap::VRAM::END) {
        return vram.read(addr - MemoryMap::VRAM::START);
    } else if (addr <= MemoryMap::RAM::END) {
        return cartridge.read(addr);
    } else if (addr <= MemoryMap::WRAM1::END) {
        return wram1.read(addr - MemoryMap::WRAM1::START);
    } else if (addr <= MemoryMap::WRAM2::END) {
        return wram2.read(addr - MemoryMap::WRAM2::START);
    } else if (addr <= MemoryMap::ECHO_RAM::END) {
        WARN() << "Read at address " + hex(addr) + " is not allowed (ECHO RAM)" << std::endl;
    } else if (addr <= MemoryMap::OAM::END) {
        return oam.read(addr - MemoryMap::OAM::START);
    } else if (addr <= MemoryMap::NOT_USABLE::END) {
        WARN() << "Read at address " + hex(addr) + " is not allowed (NOT USABLE)" << std::endl;
    } else if (addr <= MemoryMap::IO::END) {
        return (this->*(io[addr - MemoryMap::IO::START].read))();
    } else if (addr <= MemoryMap::HRAM::END) {
        return hram.read(addr - MemoryMap::HRAM::START);
    } else if (addr == MemoryMap::IE) {
        return interrupts.readIE();
    }
    return 0xFF;
}

void Bus::write(uint16_t addr, uint8_t value) {
    if (addr <= MemoryMap::ROM1::END) {
        cartridge.write(addr, value);
    } else if (addr <= MemoryMap::VRAM::END) {
        vram.write(addr - MemoryMap::VRAM::START, value);
    } else if (addr <= MemoryMap::RAM::END) {
        cartridge.write(addr, value);
    } else if (addr <= MemoryMap::WRAM1::END) {
        wram1.write(addr - MemoryMap::WRAM1::START, value);
    } else if (addr <= MemoryMap::WRAM2::END) {
        wram2.write(addr - MemoryMap::WRAM2::START, value);
    } else if (addr <= MemoryMap::ECHO_RAM::END) {
        WARN() << "Write at address " + hex(addr) + " is not allowed (ECHO RAM)" << std::endl;
    } else if (addr <= MemoryMap::OAM::END) {
        oam.write(addr - MemoryMap::OAM::START, value);
    } else if (addr <= MemoryMap::NOT_USABLE::END) {
        WARN() << "Write at address " + hex(addr) + " is not allowed (NOT USABLE)" << std::endl;
    } else if (addr <= MemoryMap::IO::END) {
        (this->*(io[addr - MemoryMap::IO::START].write))(value);
    } else if (addr <= MemoryMap::HRAM::END) {
        hram.write(addr - MemoryMap::HRAM::START, value);
    } else if (addr == MemoryMap::IE) {
        interrupts.writeIE(value);
    }
}

uint8_t Bus::readP1() const {
    return joypad.readP1();
}

void Bus::writeP1(uint8_t value) {
    return joypad.writeP1(value);
}

uint8_t Bus::readSB() const {
    return serial.readSB();
}

void Bus::writeSB(uint8_t value) {
    serial.writeSB(value);
}

uint8_t Bus::readSC() const {
    return serial.readSC();
}

void Bus::writeSC(uint8_t value) {
    serial.writeSC(value);
}

uint8_t Bus::readDIV() const {
    return timers.readDIV();
}

void Bus::writeDIV(uint8_t value) {
    timers.writeDIV(value);
}

uint8_t Bus::readTIMA() const {
    return timers.readTIMA();
}

void Bus::writeTIMA(uint8_t value) {
    timers.writeTIMA(value);
}

uint8_t Bus::readTMA() const {
    return timers.readTMA();
}

void Bus::writeTMA(uint8_t value) {
    timers.writeTMA(value);
}

uint8_t Bus::readTAC() const {
    return timers.readTAC();
}

void Bus::writeTAC(uint8_t value) {
    timers.writeTAC(value);
}

uint8_t Bus::readIF() const {
    return interrupts.readIF();
}

void Bus::writeIF(uint8_t value) {
    interrupts.writeIF(value);
}

uint8_t Bus::readIE() const {
    return interrupts.readIE();
}

void Bus::writeIE(uint8_t value) {
    interrupts.writeIE(value);
}

uint8_t Bus::readNR10() const {
    return sound.readNR10();
}

void Bus::writeNR10(uint8_t value) {
    sound.writeNR10(value);
}

uint8_t Bus::readNR11() const {
    return sound.readNR11();
}

void Bus::writeNR11(uint8_t value) {
    sound.writeNR11(value);
}

uint8_t Bus::readNR12() const {
    return sound.readNR12();
}

void Bus::writeNR12(uint8_t value) {
    sound.writeNR12(value);
}

uint8_t Bus::readNR13() const {
    return sound.readNR13();
}

void Bus::writeNR13(uint8_t value) {
    sound.writeNR13(value);
}

uint8_t Bus::readNR14() const {
    return sound.readNR14();
}

void Bus::writeNR14(uint8_t value) {
    sound.writeNR14(value);
}

uint8_t Bus::readNR21() const {
    return sound.readNR21();
}

void Bus::writeNR21(uint8_t value) {
    sound.writeNR21(value);
}

uint8_t Bus::readNR22() const {
    return sound.readNR22();
}

void Bus::writeNR22(uint8_t value) {
    sound.writeNR22(value);
}

uint8_t Bus::readNR23() const {
    return sound.readNR23();
}

void Bus::writeNR23(uint8_t value) {
    sound.writeNR23(value);
}

uint8_t Bus::readNR24() const {
    return sound.readNR24();
}

void Bus::writeNR24(uint8_t value) {
    sound.writeNR24(value);
}

uint8_t Bus::readNR30() const {
    return sound.readNR30();
}

void Bus::writeNR30(uint8_t value) {
    sound.writeNR30(value);
}

uint8_t Bus::readNR31() const {
    return sound.readNR31();
}

void Bus::writeNR31(uint8_t value) {
    sound.writeNR31(value);
}

uint8_t Bus::readNR32() const {
    return sound.readNR32();
}

void Bus::writeNR32(uint8_t value) {
    sound.writeNR32(value);
}

uint8_t Bus::readNR33() const {
    return sound.readNR33();
}

void Bus::writeNR33(uint8_t value) {
    sound.writeNR33(value);
}

uint8_t Bus::readNR34() const {
    return sound.readNR34();
}

void Bus::writeNR34(uint8_t value) {
    sound.writeNR34(value);
}

uint8_t Bus::readNR41() const {
    return sound.readNR41();
}

void Bus::writeNR41(uint8_t value) {
    sound.writeNR41(value);
}

uint8_t Bus::readNR42() const {
    return sound.readNR42();
}

void Bus::writeNR42(uint8_t value) {
    sound.writeNR42(value);
}

uint8_t Bus::readNR43() const {
    return sound.readNR43();
}

void Bus::writeNR43(uint8_t value) {
    sound.writeNR43(value);
}

uint8_t Bus::readNR44() const {
    return sound.readNR44();
}

void Bus::writeNR44(uint8_t value) {
    sound.writeNR44(value);
}

uint8_t Bus::readNR50() const {
    return sound.readNR50();
}

void Bus::writeNR50(uint8_t value) {
    sound.writeNR50(value);
}

uint8_t Bus::readNR51() const {
    return sound.readNR51();
}

void Bus::writeNR51(uint8_t value) {
    sound.writeNR51(value);
}

uint8_t Bus::readNR52() const {
    return sound.readNR52();
}

void Bus::writeNR52(uint8_t value) {
    sound.writeNR52(value);
}

uint8_t Bus::readWAVE0() const {
    return sound.readWAVE0();
}

void Bus::writeWAVE0(uint8_t value) {
    sound.writeWAVE0(value);
}

uint8_t Bus::readWAVE1() const {
    return sound.readWAVE1();
}

void Bus::writeWAVE1(uint8_t value) {
    sound.writeWAVE1(value);
}

uint8_t Bus::readWAVE2() const {
    return sound.readWAVE2();
}

void Bus::writeWAVE2(uint8_t value) {
    sound.writeWAVE2(value);
}

uint8_t Bus::readWAVE3() const {
    return sound.readWAVE3();
}

void Bus::writeWAVE3(uint8_t value) {
    sound.writeWAVE3(value);
}

uint8_t Bus::readWAVE4() const {
    return sound.readWAVE4();
}

void Bus::writeWAVE4(uint8_t value) {
    sound.writeWAVE4(value);
}

uint8_t Bus::readWAVE5() const {
    return sound.readWAVE5();
}

void Bus::writeWAVE5(uint8_t value) {
    sound.writeWAVE5(value);
}

uint8_t Bus::readWAVE6() const {
    return sound.readWAVE6();
}

void Bus::writeWAVE6(uint8_t value) {
    sound.writeWAVE6(value);
}

uint8_t Bus::readWAVE7() const {
    return sound.readWAVE7();
}

void Bus::writeWAVE7(uint8_t value) {
    sound.writeWAVE7(value);
}

uint8_t Bus::readWAVE8() const {
    return sound.readWAVE8();
}

void Bus::writeWAVE8(uint8_t value) {
    sound.writeWAVE8(value);
}

uint8_t Bus::readWAVE9() const {
    return sound.readWAVE9();
}

void Bus::writeWAVE9(uint8_t value) {
    sound.writeWAVE9(value);
}

uint8_t Bus::readWAVEA() const {
    return sound.readWAVEA();
}

void Bus::writeWAVEA(uint8_t value) {
    sound.writeWAVEA(value);
}

uint8_t Bus::readWAVEB() const {
    return sound.readWAVEB();
}

void Bus::writeWAVEB(uint8_t value) {
    sound.writeWAVEB(value);
}

uint8_t Bus::readWAVEC() const {
    return sound.readWAVEC();
}

void Bus::writeWAVEC(uint8_t value) {
    sound.writeWAVEC(value);
}

uint8_t Bus::readWAVED() const {
    return sound.readWAVED();
}

void Bus::writeWAVED(uint8_t value) {
    sound.writeWAVED(value);
}

uint8_t Bus::readWAVEE() const {
    return sound.readWAVEE();
}

void Bus::writeWAVEE(uint8_t value) {
    sound.writeWAVEE(value);
}

uint8_t Bus::readWAVEF() const {
    return sound.readWAVEF();
}

void Bus::writeWAVEF(uint8_t value) {
    sound.writeWAVEF(value);
}

uint8_t Bus::readLCDC() const {
    return lcd.readLCDC();
}

void Bus::writeLCDC(uint8_t value) {
    lcd.writeLCDC(value);
}

uint8_t Bus::readSTAT() const {
    return lcd.readSTAT();
}

void Bus::writeSTAT(uint8_t value) {
    lcd.writeSTAT(value);
}

uint8_t Bus::readSCY() const {
    return lcd.readSCY();
}

void Bus::writeSCY(uint8_t value) {
    lcd.writeSCY(value);
}

uint8_t Bus::readSCX() const {
    return lcd.readSCX();
}

void Bus::writeSCX(uint8_t value) {
    lcd.writeSCX(value);
}

uint8_t Bus::readLY() const {
    return lcd.readLY();
}

void Bus::writeLY(uint8_t value) {
    lcd.writeLY(value);
}

uint8_t Bus::readLYC() const {
    return lcd.readLYC();
}

void Bus::writeLYC(uint8_t value) {
    lcd.writeLYC(value);
}

uint8_t Bus::readDMA() const {
    return lcd.readDMA();
}

void Bus::writeDMA(uint8_t value) {
    lcd.writeDMA(value);
}

uint8_t Bus::readBGP() const {
    return lcd.readBGP();
}

void Bus::writeBGP(uint8_t value) {
    lcd.writeBGP(value);
}

uint8_t Bus::readOBP0() const {
    return lcd.readOBP0();
}

void Bus::writeOBP0(uint8_t value) {
    lcd.writeOBP0(value);
}

uint8_t Bus::readOBP1() const {
    return lcd.readOBP1();
}

void Bus::writeOBP1(uint8_t value) {
    lcd.writeOBP1(value);
}

uint8_t Bus::readWY() const {
    return lcd.readWY();
}

void Bus::writeWY(uint8_t value) {
    lcd.writeWY(value);
}

uint8_t Bus::readWX() const {
    return lcd.readWX();
}

void Bus::writeWX(uint8_t value) {
    lcd.writeWX(value);
}

uint8_t Bus::readBOOT() const {
    return boot.readBOOT();
}

void Bus::writeBOOT(uint8_t value) {
    boot.writeBOOT(value);
}

uint8_t Bus::readNull() const {
    return 0xFF;
}

void Bus::writeNull(uint8_t value) {
}
