#include "bus.h"
#include "docboy/boot/boot.h"
#include "docboy/cartridge/slot.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/memory/memory.hpp"
#include "docboy/ppu/video.h"
#include "docboy/serial/serial.h"
#include "docboy/shared/specs.h"
#include "docboy/sound/sound.h"
#include "docboy/timers/timers.h"
#include "utils/formatters.hpp"
#include "utils/log.h"

Bus::MemoryAccess::MemoryAccess(byte* rw) {
    read.trivial = write.trivial = rw;
}

Bus::MemoryAccess::MemoryAccess(byte* r, byte* w) {
    read.trivial = r;
    write.trivial = w;
}

Bus::MemoryAccess::MemoryAccess(Bus::NonTrivialMemoryRead r, byte* w) {
    read.nonTrivial = r;
    read.trivial = w;
}

Bus::MemoryAccess::MemoryAccess(byte* r, Bus::NonTrivialMemoryWrite w) {
    read.trivial = r;
    write.nonTrivial = w;
}

Bus::MemoryAccess::MemoryAccess(Bus::NonTrivialMemoryRead r, Bus::NonTrivialMemoryWrite w) {
    read.nonTrivial = r;
    write.nonTrivial = w;
}

Bus::Bus(IF_BOOTROM(BootRom& bootRom COMMA) CartridgeSlot& cartridgeSlot, Vram& vram, Wram1& wram1, Wram2& wram2,
         Oam& oam, Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers, InterruptsIO& interrupts,
         SoundIO& sound, VideoIO& video, BootIO& boot) :
    IF_BOOTROM(bootRom(bootRom) COMMA) cartridgeSlot(cartridgeSlot),
    vram(vram),
    wram1(wram1),
    wram2(wram2),
    oam(oam),
    hram(hram),
    joypad(joypad),
    serial(serial),
    timers(timers),
    interrupts(interrupts),
    sound(sound),
    video(video),
    boot(boot) {

    const MemoryAccess null {&Bus::readNull, &Bus::writeNull};

    /* 0x0000 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::ROM0::START; i <= Specs::MemoryLayout::ROM1::END; i++) {
        memoryAccessors[i] = {&Bus::readCartridgeRom, &Bus::writeCartridgeRom};
    }

    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memoryAccessors[i] = &vram[i - Specs::MemoryLayout::VRAM::START];
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        memoryAccessors[i] = {&Bus::readCartridgeRam, &Bus::writeCartridgeRam};
    }

    /* 0xC000 - 0xCFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM1::START; i <= Specs::MemoryLayout::WRAM1::END; i++) {
        memoryAccessors[i] = &wram1[i - Specs::MemoryLayout::WRAM1::START];
    }

    /* 0xD000 - 0xDFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
        memoryAccessors[i] = &wram2[i - Specs::MemoryLayout::WRAM2::START];
    }

    /* 0xE000 - 0xFDFF */
    for (uint16_t i = Specs::MemoryLayout::ECHO_RAM::START; i <= Specs::MemoryLayout::ECHO_RAM::END; i++) {
        memoryAccessors[i] = null;
    }

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memoryAccessors[i] = &oam[i - Specs::MemoryLayout::OAM::START];
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memoryAccessors[i] = null;
    }

    /* FF00 */ memoryAccessors[Specs::Registers::Joypad::P1] = {&Bus::readP1, &Bus::writeP1};
    /* FF01 */ memoryAccessors[Specs::Registers::Serial::SB] = &serial.SB;
    /* FF02 */ memoryAccessors[Specs::Registers::Serial::SC] = {&serial.SC, &Bus::writeSC};
    /* FF03 */ memoryAccessors[0xFF03] = null;
    /* FF04 */ memoryAccessors[Specs::Registers::Timers::DIV] = {&Bus::readDIV, &Bus::writeDIV};
    /* FF05 */ memoryAccessors[Specs::Registers::Timers::TIMA] = {&timers.TIMA, &Bus::writeTIMA};
    /* FF06 */ memoryAccessors[Specs::Registers::Timers::TMA] = {&timers.TMA, &Bus::writeTMA};
    /* FF07 */ memoryAccessors[Specs::Registers::Timers::TAC] = {&timers.TAC, &Bus::writeTAC};
    /* FF08 */ memoryAccessors[0xFF08] = null;
    /* FF09 */ memoryAccessors[0xFF09] = null;
    /* FF0A */ memoryAccessors[0xFF0A] = null;
    /* FF0B */ memoryAccessors[0xFF0B] = null;
    /* FF0C */ memoryAccessors[0xFF0C] = null;
    /* FF0D */ memoryAccessors[0xFF0D] = null;
    /* FF0E */ memoryAccessors[0xFF0E] = null;
    /* FF0F */ memoryAccessors[Specs::Registers::Interrupts::IF] = {&interrupts.IF, &Bus::writeIF};
    /* FF10 */ memoryAccessors[Specs::Registers::Sound::NR10] = {&sound.NR10, &Bus::writeNR10};
    /* FF11 */ memoryAccessors[Specs::Registers::Sound::NR11] = &sound.NR11;
    /* FF12 */ memoryAccessors[Specs::Registers::Sound::NR12] = &sound.NR12;
    /* FF13 */ memoryAccessors[Specs::Registers::Sound::NR13] = &sound.NR13;
    /* FF14 */ memoryAccessors[Specs::Registers::Sound::NR14] = &sound.NR14;
    /* FF15 */ memoryAccessors[0xFF15] = null;
    /* FF16 */ memoryAccessors[Specs::Registers::Sound::NR21] = &sound.NR21;
    /* FF17 */ memoryAccessors[Specs::Registers::Sound::NR22] = &sound.NR22;
    /* FF18 */ memoryAccessors[Specs::Registers::Sound::NR23] = &sound.NR23;
    /* FF19 */ memoryAccessors[Specs::Registers::Sound::NR24] = &sound.NR24;
    /* FF1A */ memoryAccessors[Specs::Registers::Sound::NR30] = {&sound.NR30, &Bus::writeNR30};
    /* FF1B */ memoryAccessors[Specs::Registers::Sound::NR31] = &sound.NR31;
    /* FF1C */ memoryAccessors[Specs::Registers::Sound::NR32] = {&sound.NR32, &Bus::writeNR32};
    /* FF1D */ memoryAccessors[Specs::Registers::Sound::NR33] = &sound.NR33;
    /* FF1E */ memoryAccessors[Specs::Registers::Sound::NR34] = &sound.NR34;
    /* FF1F */ memoryAccessors[0xFF1F] = null;
    /* FF20 */ memoryAccessors[Specs::Registers::Sound::NR41] = {&sound.NR41, &Bus::writeNR41};
    /* FF21 */ memoryAccessors[Specs::Registers::Sound::NR42] = &sound.NR42;
    /* FF22 */ memoryAccessors[Specs::Registers::Sound::NR43] = &sound.NR43;
    /* FF23 */ memoryAccessors[Specs::Registers::Sound::NR44] = {&sound.NR44, &Bus::writeNR44};
    /* FF24 */ memoryAccessors[Specs::Registers::Sound::NR50] = &sound.NR50;
    /* FF25 */ memoryAccessors[Specs::Registers::Sound::NR51] = &sound.NR51;
    /* FF26 */ memoryAccessors[Specs::Registers::Sound::NR52] = {&sound.NR52, &Bus::writeNR52};
    /* FF27 */ memoryAccessors[0xFF27] = null;
    /* FF28 */ memoryAccessors[0xFF28] = null;
    /* FF29 */ memoryAccessors[0xFF29] = null;
    /* FF2A */ memoryAccessors[0xFF2A] = null;
    /* FF2B */ memoryAccessors[0xFF2B] = null;
    /* FF2C */ memoryAccessors[0xFF2C] = null;
    /* FF2D */ memoryAccessors[0xFF2D] = null;
    /* FF2E */ memoryAccessors[0xFF2E] = null;
    /* FF2F */ memoryAccessors[0xFF2F] = null;
    /* FF30 */ memoryAccessors[Specs::Registers::Sound::WAVE0] = &sound.WAVE0;
    /* FF31 */ memoryAccessors[Specs::Registers::Sound::WAVE1] = &sound.WAVE1;
    /* FF32 */ memoryAccessors[Specs::Registers::Sound::WAVE2] = &sound.WAVE2;
    /* FF33 */ memoryAccessors[Specs::Registers::Sound::WAVE3] = &sound.WAVE3;
    /* FF34 */ memoryAccessors[Specs::Registers::Sound::WAVE4] = &sound.WAVE4;
    /* FF35 */ memoryAccessors[Specs::Registers::Sound::WAVE5] = &sound.WAVE5;
    /* FF36 */ memoryAccessors[Specs::Registers::Sound::WAVE6] = &sound.WAVE6;
    /* FF37 */ memoryAccessors[Specs::Registers::Sound::WAVE7] = &sound.WAVE7;
    /* FF38 */ memoryAccessors[Specs::Registers::Sound::WAVE8] = &sound.WAVE8;
    /* FF39 */ memoryAccessors[Specs::Registers::Sound::WAVE9] = &sound.WAVE9;
    /* FF3A */ memoryAccessors[Specs::Registers::Sound::WAVEA] = &sound.WAVEA;
    /* FF3B */ memoryAccessors[Specs::Registers::Sound::WAVEB] = &sound.WAVEB;
    /* FF3C */ memoryAccessors[Specs::Registers::Sound::WAVEC] = &sound.WAVEC;
    /* FF3D */ memoryAccessors[Specs::Registers::Sound::WAVED] = &sound.WAVED;
    /* FF3E */ memoryAccessors[Specs::Registers::Sound::WAVEE] = &sound.WAVEE;
    /* FF3F */ memoryAccessors[Specs::Registers::Sound::WAVEF] = &sound.WAVEF;
    /* FF40 */ memoryAccessors[Specs::Registers::Video::LCDC] = &video.LCDC;
    /* FF41 */ memoryAccessors[Specs::Registers::Video::STAT] = {&video.STAT, &Bus::writeSTAT};
    /* FF42 */ memoryAccessors[Specs::Registers::Video::SCY] = &video.SCY;
    /* FF43 */ memoryAccessors[Specs::Registers::Video::SCX] = &video.SCX;
    /* FF44 */ memoryAccessors[Specs::Registers::Video::LY] = &video.LY;
    /* FF45 */ memoryAccessors[Specs::Registers::Video::LYC] = &video.LYC;
    /* FF46 */ memoryAccessors[Specs::Registers::Video::DMA] = {&video.DMA, &Bus::writeDMA};
    /* FF47 */ memoryAccessors[Specs::Registers::Video::BGP] = &video.BGP;
    /* FF48 */ memoryAccessors[Specs::Registers::Video::OBP0] = &video.OBP0;
    /* FF49 */ memoryAccessors[Specs::Registers::Video::OBP1] = &video.OBP1;
    /* FF4A */ memoryAccessors[Specs::Registers::Video::WY] = &video.WY;
    /* FF4B */ memoryAccessors[Specs::Registers::Video::WX] = &video.WX;
    /* FF4C */ memoryAccessors[0xFF4C] = null;
    /* FF4D */ memoryAccessors[0xFF4D] = null;
    /* FF4E */ memoryAccessors[0xFF4E] = null;
    /* FF4F */ memoryAccessors[0xFF4F] = null;
    /* FF50 */ memoryAccessors[Specs::Registers::Boot::BOOT] = {&boot.BOOT, &Bus::writeBOOT};
    /* FF51 */ memoryAccessors[0xFF51] = null;
    /* FF52 */ memoryAccessors[0xFF52] = null;
    /* FF53 */ memoryAccessors[0xFF53] = null;
    /* FF54 */ memoryAccessors[0xFF54] = null;
    /* FF55 */ memoryAccessors[0xFF55] = null;
    /* FF56 */ memoryAccessors[0xFF56] = null;
    /* FF57 */ memoryAccessors[0xFF57] = null;
    /* FF58 */ memoryAccessors[0xFF58] = null;
    /* FF59 */ memoryAccessors[0xFF59] = null;
    /* FF5A */ memoryAccessors[0xFF5A] = null;
    /* FF5B */ memoryAccessors[0xFF5B] = null;
    /* FF5C */ memoryAccessors[0xFF5C] = null;
    /* FF5D */ memoryAccessors[0xFF5D] = null;
    /* FF5E */ memoryAccessors[0xFF5E] = null;
    /* FF5F */ memoryAccessors[0xFF5F] = null;
    /* FF60 */ memoryAccessors[0xFF60] = null;
    /* FF61 */ memoryAccessors[0xFF61] = null;
    /* FF62 */ memoryAccessors[0xFF62] = null;
    /* FF63 */ memoryAccessors[0xFF63] = null;
    /* FF64 */ memoryAccessors[0xFF64] = null;
    /* FF65 */ memoryAccessors[0xFF65] = null;
    /* FF66 */ memoryAccessors[0xFF66] = null;
    /* FF67 */ memoryAccessors[0xFF67] = null;
    /* FF68 */ memoryAccessors[0xFF68] = null;
    /* FF69 */ memoryAccessors[0xFF69] = null;
    /* FF6A */ memoryAccessors[0xFF6A] = null;
    /* FF6B */ memoryAccessors[0xFF6B] = null;
    /* FF6C */ memoryAccessors[0xFF6C] = null;
    /* FF6D */ memoryAccessors[0xFF6D] = null;
    /* FF6E */ memoryAccessors[0xFF6E] = null;
    /* FF6F */ memoryAccessors[0xFF6F] = null;
    /* FF70 */ memoryAccessors[0xFF70] = null;
    /* FF71 */ memoryAccessors[0xFF71] = null;
    /* FF72 */ memoryAccessors[0xFF72] = null;
    /* FF73 */ memoryAccessors[0xFF73] = null;
    /* FF74 */ memoryAccessors[0xFF74] = null;
    /* FF75 */ memoryAccessors[0xFF75] = null;
    /* FF76 */ memoryAccessors[0xFF76] = null;
    /* FF77 */ memoryAccessors[0xFF77] = null;
    /* FF78 */ memoryAccessors[0xFF78] = null;
    /* FF79 */ memoryAccessors[0xFF79] = null;
    /* FF7A */ memoryAccessors[0xFF7A] = null;
    /* FF7B */ memoryAccessors[0xFF7B] = null;
    /* FF7C */ memoryAccessors[0xFF7C] = null;
    /* FF7D */ memoryAccessors[0xFF7D] = null;
    /* FF7E */ memoryAccessors[0xFF7E] = null;
    /* FF7F */ memoryAccessors[0xFF7F] = null;

    /* 0xFF80 - 0xFFFE */
    for (uint16_t i = Specs::MemoryLayout::HRAM::START; i <= Specs::MemoryLayout::HRAM::END; i++) {
        memoryAccessors[i] = &hram[i - Specs::MemoryLayout::HRAM::START];
    }

    /* FFFF */ memoryAccessors[Specs::MemoryLayout::IE] = &interrupts.IE;
}

uint8_t Bus::read(uint16_t address) const {
    const MemoryAccess::Read& memRead = memoryAccessors[address].read;
    return memRead.trivial ? static_cast<uint8_t>(*memRead.trivial) : (this->*(memRead.nonTrivial))(address);
}

void Bus::write(uint16_t address, uint8_t value) {
    const MemoryAccess::Write& memoryAccess = memoryAccessors[address].write;
    if (memoryAccess.trivial) {
        *memoryAccess.trivial = value;
    } else {
        (this->*(memoryAccess.nonTrivial))(address, value);
    }
}

uint8_t Bus::readCartridgeRom(uint16_t address) const {
#ifdef ENABLE_BOOTROM
    if (test_bit<Specs::Bits::Boot::BOOT_ENABLE> == 0 && address <= Specs::MemoryLayout::BOOTROM::END) {
        return bootRom[address];
    }
#endif

    return cartridgeSlot.readRom(address);
}

void Bus::writeCartridgeRom(uint16_t address, uint8_t value) {
    return cartridgeSlot.writeRom(address, value);
}

uint8_t Bus::readCartridgeRam(uint16_t address) const {
    return cartridgeSlot.readRam(address);
}

void Bus::writeCartridgeRam(uint16_t address, uint8_t value) {
    return cartridgeSlot.writeRam(address, value);
}

uint8_t Bus::readP1(uint16_t address) const {
    return joypad.readP1();
}

void Bus::writeP1(uint16_t address, uint8_t value) {
    return joypad.writeP1(value);
}

void Bus::writeSC(uint16_t address, uint8_t value) {
    return serial.writeSC(value);
}

uint8_t Bus::readDIV(uint16_t address) const {
    return timers.readDIV();
}

void Bus::writeDIV(uint16_t address, uint8_t value) {
    timers.writeDIV(value);
}

void Bus::writeTIMA(uint16_t address, uint8_t value) {
    timers.writeTIMA(value);
}

void Bus::writeTMA(uint16_t address, uint8_t value) {
    timers.writeTMA(value);
}

void Bus::writeTAC(uint16_t address, uint8_t value) {
    timers.writeTAC(value);
}

void Bus::writeIF(uint16_t address, uint8_t value) {
    interrupts.writeIF(value);
}

void Bus::writeNR10(uint16_t address, uint8_t value) {
    sound.writeNR10(value);
}

void Bus::writeNR30(uint16_t address, uint8_t value) {
    sound.writeNR30(value);
}

void Bus::writeNR32(uint16_t address, uint8_t value) {
    sound.writeNR32(value);
}

void Bus::writeNR41(uint16_t address, uint8_t value) {
    sound.writeNR41(value);
}

void Bus::writeNR44(uint16_t address, uint8_t value) {
    sound.writeNR44(value);
}

void Bus::writeNR52(uint16_t address, uint8_t value) {
    sound.writeNR52(value);
}

void Bus::writeSTAT(uint16_t address, uint8_t value) {
    video.writeSTAT(value);
}

void Bus::writeDMA(uint16_t address, uint8_t value) {
    video.writeDMA(value);
}

void Bus::writeBOOT(uint16_t address, uint8_t value) {
    boot.writeBOOT(value);
}

uint8_t Bus::readNull(uint16_t address) const {
    return 0xFF;
}

void Bus::writeNull(uint16_t address, uint8_t value) {
}
