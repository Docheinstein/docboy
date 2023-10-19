#include "bus.h"
#include "docboy/boot/boot.h"
#include "docboy/cartridge/slot.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/memory/memory.hpp"
#include "docboy/memory/null.h"
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
         SoundIO& sound, VideoIO& video, BootIO& boot, NullIO& nullIO) :
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
    boot(boot),
    nullIO(nullIO) {

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
        memoryAccessors[i] = {&Bus::readEchoRam, &Bus::writeEchoRam};
    }

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memoryAccessors[i] = &oam[i - Specs::MemoryLayout::OAM::START];
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memoryAccessors[i] = {&Bus::readNotUsableArea, &Bus::writeNotUsableArea};
    }

    /* FF00 */ memoryAccessors[Specs::Registers::Joypad::P1] = {&Bus::readP1, &Bus::writeP1};
    /* FF01 */ memoryAccessors[Specs::Registers::Serial::SB] = &serial.SB;
    /* FF02 */ memoryAccessors[Specs::Registers::Serial::SC] = &serial.SC;
    /* FF03 */ memoryAccessors[0xFF03] = &nullIO.FF03;
    // TODO: timers write are non trivial
    /* FF04 */ memoryAccessors[Specs::Registers::Timers::DIV] = {&Bus::readDIV, &Bus::writeDIV};
    /* FF05 */ memoryAccessors[Specs::Registers::Timers::TIMA] = &timers.TIMA;
    /* FF06 */ memoryAccessors[Specs::Registers::Timers::TMA] = &timers.TMA;
    /* FF07 */ memoryAccessors[Specs::Registers::Timers::TAC] = {&timers.TAC, &Bus::writeTAC};
    /* FF08 */ memoryAccessors[0xFF08] = &nullIO.FF08;
    /* FF09 */ memoryAccessors[0xFF09] = &nullIO.FF09;
    /* FF0A */ memoryAccessors[0xFF0A] = &nullIO.FF0A;
    /* FF0B */ memoryAccessors[0xFF0B] = &nullIO.FF0B;
    /* FF0C */ memoryAccessors[0xFF0C] = &nullIO.FF0C;
    /* FF0D */ memoryAccessors[0xFF0D] = &nullIO.FF0D;
    /* FF0E */ memoryAccessors[0xFF0E] = &nullIO.FF0E;
    /* FF0F */ memoryAccessors[Specs::Registers::Interrupts::IF] = &interrupts.IF;
    /* FF10 */ memoryAccessors[Specs::Registers::Sound::NR10] = &sound.NR10;
    /* FF11 */ memoryAccessors[Specs::Registers::Sound::NR11] = &sound.NR11;
    /* FF12 */ memoryAccessors[Specs::Registers::Sound::NR12] = &sound.NR12;
    /* FF13 */ memoryAccessors[Specs::Registers::Sound::NR13] = &sound.NR13;
    /* FF14 */ memoryAccessors[Specs::Registers::Sound::NR14] = &sound.NR14;
    /* FF15 */ memoryAccessors[0xFF15] = &nullIO.FF15;
    /* FF16 */ memoryAccessors[Specs::Registers::Sound::NR21] = &sound.NR21;
    /* FF17 */ memoryAccessors[Specs::Registers::Sound::NR22] = &sound.NR22;
    /* FF18 */ memoryAccessors[Specs::Registers::Sound::NR23] = &sound.NR23;
    /* FF19 */ memoryAccessors[Specs::Registers::Sound::NR24] = &sound.NR24;
    /* FF1A */ memoryAccessors[Specs::Registers::Sound::NR30] = &sound.NR30;
    /* FF1B */ memoryAccessors[Specs::Registers::Sound::NR31] = &sound.NR31;
    /* FF1C */ memoryAccessors[Specs::Registers::Sound::NR32] = &sound.NR32;
    /* FF1D */ memoryAccessors[Specs::Registers::Sound::NR33] = &sound.NR33;
    /* FF1E */ memoryAccessors[Specs::Registers::Sound::NR34] = &sound.NR34;
    /* FF1F */ memoryAccessors[0xFF1F] = &nullIO.FF1F;
    /* FF20 */ memoryAccessors[Specs::Registers::Sound::NR41] = &sound.NR41;
    /* FF21 */ memoryAccessors[Specs::Registers::Sound::NR42] = &sound.NR42;
    /* FF22 */ memoryAccessors[Specs::Registers::Sound::NR43] = &sound.NR43;
    /* FF23 */ memoryAccessors[Specs::Registers::Sound::NR44] = &sound.NR44;
    /* FF24 */ memoryAccessors[Specs::Registers::Sound::NR50] = &sound.NR50;
    /* FF25 */ memoryAccessors[Specs::Registers::Sound::NR51] = &sound.NR51;
    /* FF26 */ memoryAccessors[Specs::Registers::Sound::NR52] = &sound.NR52;
    /* FF27 */ memoryAccessors[0xFF27] = &nullIO.FF27;
    /* FF28 */ memoryAccessors[0xFF28] = &nullIO.FF28;
    /* FF29 */ memoryAccessors[0xFF29] = &nullIO.FF29;
    /* FF2A */ memoryAccessors[0xFF2A] = &nullIO.FF2A;
    /* FF2B */ memoryAccessors[0xFF2B] = &nullIO.FF2B;
    /* FF2C */ memoryAccessors[0xFF2C] = &nullIO.FF2C;
    /* FF2D */ memoryAccessors[0xFF2D] = &nullIO.FF2D;
    /* FF2E */ memoryAccessors[0xFF2E] = &nullIO.FF2E;
    /* FF2F */ memoryAccessors[0xFF2F] = &nullIO.FF2F;
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
    /* FF41 */ memoryAccessors[Specs::Registers::Video::STAT] = &video.STAT;
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
    /* FF4C */ memoryAccessors[0xFF4C] = &nullIO.FF4C;
    /* FF4D */ memoryAccessors[0xFF4D] = &nullIO.FF4D;
    /* FF4E */ memoryAccessors[0xFF4E] = &nullIO.FF4E;
    /* FF4F */ memoryAccessors[0xFF4F] = &nullIO.FF4F;
    /* FF50 */ memoryAccessors[Specs::Registers::Boot::BOOT] = &boot.BOOT;
    /* FF51 */ memoryAccessors[0xFF51] = &nullIO.FF51;
    /* FF52 */ memoryAccessors[0xFF52] = &nullIO.FF52;
    /* FF53 */ memoryAccessors[0xFF53] = &nullIO.FF53;
    /* FF54 */ memoryAccessors[0xFF54] = &nullIO.FF54;
    /* FF55 */ memoryAccessors[0xFF55] = &nullIO.FF55;
    /* FF56 */ memoryAccessors[0xFF56] = &nullIO.FF56;
    /* FF57 */ memoryAccessors[0xFF57] = &nullIO.FF57;
    /* FF58 */ memoryAccessors[0xFF58] = &nullIO.FF58;
    /* FF59 */ memoryAccessors[0xFF59] = &nullIO.FF59;
    /* FF5A */ memoryAccessors[0xFF5A] = &nullIO.FF5A;
    /* FF5B */ memoryAccessors[0xFF5B] = &nullIO.FF5B;
    /* FF5C */ memoryAccessors[0xFF5C] = &nullIO.FF5C;
    /* FF5D */ memoryAccessors[0xFF5D] = &nullIO.FF5D;
    /* FF5E */ memoryAccessors[0xFF5E] = &nullIO.FF5E;
    /* FF5F */ memoryAccessors[0xFF5F] = &nullIO.FF5F;
    /* FF60 */ memoryAccessors[0xFF60] = &nullIO.FF60;
    /* FF61 */ memoryAccessors[0xFF61] = &nullIO.FF61;
    /* FF62 */ memoryAccessors[0xFF62] = &nullIO.FF62;
    /* FF63 */ memoryAccessors[0xFF63] = &nullIO.FF63;
    /* FF64 */ memoryAccessors[0xFF64] = &nullIO.FF64;
    /* FF65 */ memoryAccessors[0xFF65] = &nullIO.FF65;
    /* FF66 */ memoryAccessors[0xFF66] = &nullIO.FF66;
    /* FF67 */ memoryAccessors[0xFF67] = &nullIO.FF67;
    /* FF68 */ memoryAccessors[0xFF68] = &nullIO.FF68;
    /* FF69 */ memoryAccessors[0xFF69] = &nullIO.FF69;
    /* FF6A */ memoryAccessors[0xFF6A] = &nullIO.FF6A;
    /* FF6B */ memoryAccessors[0xFF6B] = &nullIO.FF6B;
    /* FF6C */ memoryAccessors[0xFF6C] = &nullIO.FF6C;
    /* FF6D */ memoryAccessors[0xFF6D] = &nullIO.FF6D;
    /* FF6E */ memoryAccessors[0xFF6E] = &nullIO.FF6E;
    /* FF6F */ memoryAccessors[0xFF6F] = &nullIO.FF6F;
    /* FF70 */ memoryAccessors[0xFF70] = &nullIO.FF70;
    /* FF71 */ memoryAccessors[0xFF71] = &nullIO.FF71;
    /* FF72 */ memoryAccessors[0xFF72] = &nullIO.FF72;
    /* FF73 */ memoryAccessors[0xFF73] = &nullIO.FF73;
    /* FF74 */ memoryAccessors[0xFF74] = &nullIO.FF74;
    /* FF75 */ memoryAccessors[0xFF75] = &nullIO.FF75;
    /* FF76 */ memoryAccessors[0xFF76] = &nullIO.FF76;
    /* FF77 */ memoryAccessors[0xFF77] = &nullIO.FF77;
    /* FF78 */ memoryAccessors[0xFF78] = &nullIO.FF78;
    /* FF79 */ memoryAccessors[0xFF79] = &nullIO.FF79;
    /* FF7A */ memoryAccessors[0xFF7A] = &nullIO.FF7A;
    /* FF7B */ memoryAccessors[0xFF7B] = &nullIO.FF7B;
    /* FF7C */ memoryAccessors[0xFF7C] = &nullIO.FF7C;
    /* FF7D */ memoryAccessors[0xFF7D] = &nullIO.FF7D;
    /* FF7E */ memoryAccessors[0xFF7E] = &nullIO.FF7E;
    /* FF7F */ memoryAccessors[0xFF7F] = &nullIO.FF7F;

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
    if (boot.BOOT == 0 && address <= Specs::MemoryLayout::BOOTROM::END) {
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

uint8_t Bus::readEchoRam(uint16_t address) const {
    WARN("EchoRam: read at address " + hex(address) + " is not allowed")
    return 0xFF;
}

void Bus::writeEchoRam(uint16_t address,
                       uint8_t value) {WARN("EchoRam: write at address" + hex(address) + " is not allowed")}

uint8_t Bus::readNotUsableArea(uint16_t address) const {
    WARN("NotUsable: read at address " + hex(address) + " is not allowed")
    return 0xFF;
}

void Bus::writeNotUsableArea(uint16_t address,
                             uint8_t value) {WARN("NotUsable: write at address " + hex(address) + " is not allowed")}

uint8_t Bus::readP1(uint16_t address) const {
    return joypad.readP1();
}

void Bus::writeP1(uint16_t address, uint8_t value) {
    return joypad.writeP1(value);
}

void Bus::writeDMA(uint16_t address, uint8_t value) {
    video.writeDMA(value);
}

uint8_t Bus::readDIV(uint16_t address) const {
    return timers.readDIV();
}

void Bus::writeDIV(uint16_t address, uint8_t value) {
    timers.writeDIV(value);
}

void Bus::writeTAC(uint16_t address, uint8_t value) {
    timers.writeTAC(value);
}
