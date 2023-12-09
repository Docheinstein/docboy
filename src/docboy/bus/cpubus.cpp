#include "cpubus.h"
#include "docboy/bootrom/bootrom.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/memory/hram.h"
#include "docboy/ppu/video.h"
#include "docboy/serial/serial.h"
#include "docboy/sound/sound.h"
#include "docboy/timers/timers.h"

CpuBus::CpuBus(IF_BOOTROM(BootRom& bootRom COMMA) Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers,
               InterruptsIO& interrupts, SoundIO& sound, VideoIO& video, BootIO& boot) :
    Bus<CpuBus>(),
    IF_BOOTROM(bootRom(bootRom) COMMA) hram(hram),
    io({joypad, serial, timers, interrupts, sound, video, boot}) {

    const MemoryAccess FF {&CpuBus::readFF, &CpuBus::writeNop};

#ifdef ENABLE_BOOTROM
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        memoryAccessors[i] = &bootRom[i];
    }
#endif

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memoryAccessors[i] = FF;
    }

    /* FF00 */ memoryAccessors[Specs::Registers::Joypad::P1] = {&CpuBus::readP1, &CpuBus::writeP1};
    /* FF01 */ memoryAccessors[Specs::Registers::Serial::SB] = &io.serial.SB;
    /* FF02 */ memoryAccessors[Specs::Registers::Serial::SC] = {&io.serial.SC, &CpuBus::writeSC};
    /* FF03 */ memoryAccessors[0xFF03] = FF;
    /* FF04 */ memoryAccessors[Specs::Registers::Timers::DIV] = {&CpuBus::readDIV, &CpuBus::writeDIV};
    /* FF05 */ memoryAccessors[Specs::Registers::Timers::TIMA] = {&io.timers.TIMA, &CpuBus::writeTIMA};
    /* FF06 */ memoryAccessors[Specs::Registers::Timers::TMA] = {&io.timers.TMA, &CpuBus::writeTMA};
    /* FF07 */ memoryAccessors[Specs::Registers::Timers::TAC] = {&io.timers.TAC, &CpuBus::writeTAC};
    /* FF08 */ memoryAccessors[0xFF08] = FF;
    /* FF09 */ memoryAccessors[0xFF09] = FF;
    /* FF0A */ memoryAccessors[0xFF0A] = FF;
    /* FF0B */ memoryAccessors[0xFF0B] = FF;
    /* FF0C */ memoryAccessors[0xFF0C] = FF;
    /* FF0D */ memoryAccessors[0xFF0D] = FF;
    /* FF0E */ memoryAccessors[0xFF0E] = FF;
    /* FF0F */ memoryAccessors[Specs::Registers::Interrupts::IF] = {&io.interrupts.IF, &CpuBus::writeIF};
    /* FF10 */ memoryAccessors[Specs::Registers::Sound::NR10] = {&io.sound.NR10, &CpuBus::writeNR10};
    /* FF11 */ memoryAccessors[Specs::Registers::Sound::NR11] = &io.sound.NR11;
    /* FF12 */ memoryAccessors[Specs::Registers::Sound::NR12] = &io.sound.NR12;
    /* FF13 */ memoryAccessors[Specs::Registers::Sound::NR13] = &io.sound.NR13;
    /* FF14 */ memoryAccessors[Specs::Registers::Sound::NR14] = &io.sound.NR14;
    /* FF15 */ memoryAccessors[0xFF15] = FF;
    /* FF16 */ memoryAccessors[Specs::Registers::Sound::NR21] = &io.sound.NR21;
    /* FF17 */ memoryAccessors[Specs::Registers::Sound::NR22] = &io.sound.NR22;
    /* FF18 */ memoryAccessors[Specs::Registers::Sound::NR23] = &io.sound.NR23;
    /* FF19 */ memoryAccessors[Specs::Registers::Sound::NR24] = &io.sound.NR24;
    /* FF1A */ memoryAccessors[Specs::Registers::Sound::NR30] = {&io.sound.NR30, &CpuBus::writeNR30};
    /* FF1B */ memoryAccessors[Specs::Registers::Sound::NR31] = &io.sound.NR31;
    /* FF1C */ memoryAccessors[Specs::Registers::Sound::NR32] = {&io.sound.NR32, &CpuBus::writeNR32};
    /* FF1D */ memoryAccessors[Specs::Registers::Sound::NR33] = &io.sound.NR33;
    /* FF1E */ memoryAccessors[Specs::Registers::Sound::NR34] = &io.sound.NR34;
    /* FF1F */ memoryAccessors[0xFF1F] = FF;
    /* FF20 */ memoryAccessors[Specs::Registers::Sound::NR41] = {&io.sound.NR41, &CpuBus::writeNR41};
    /* FF21 */ memoryAccessors[Specs::Registers::Sound::NR42] = &io.sound.NR42;
    /* FF22 */ memoryAccessors[Specs::Registers::Sound::NR43] = &io.sound.NR43;
    /* FF23 */ memoryAccessors[Specs::Registers::Sound::NR44] = {&io.sound.NR44, &CpuBus::writeNR44};
    /* FF24 */ memoryAccessors[Specs::Registers::Sound::NR50] = &io.sound.NR50;
    /* FF25 */ memoryAccessors[Specs::Registers::Sound::NR51] = &io.sound.NR51;
    /* FF26 */ memoryAccessors[Specs::Registers::Sound::NR52] = {&io.sound.NR52, &CpuBus::writeNR52};
    /* FF27 */ memoryAccessors[0xFF27] = FF;
    /* FF28 */ memoryAccessors[0xFF28] = FF;
    /* FF29 */ memoryAccessors[0xFF29] = FF;
    /* FF2A */ memoryAccessors[0xFF2A] = FF;
    /* FF2B */ memoryAccessors[0xFF2B] = FF;
    /* FF2C */ memoryAccessors[0xFF2C] = FF;
    /* FF2D */ memoryAccessors[0xFF2D] = FF;
    /* FF2E */ memoryAccessors[0xFF2E] = FF;
    /* FF2F */ memoryAccessors[0xFF2F] = FF;
    /* FF30 */ memoryAccessors[Specs::Registers::Sound::WAVE0] = &io.sound.WAVE0;
    /* FF31 */ memoryAccessors[Specs::Registers::Sound::WAVE1] = &io.sound.WAVE1;
    /* FF32 */ memoryAccessors[Specs::Registers::Sound::WAVE2] = &io.sound.WAVE2;
    /* FF33 */ memoryAccessors[Specs::Registers::Sound::WAVE3] = &io.sound.WAVE3;
    /* FF34 */ memoryAccessors[Specs::Registers::Sound::WAVE4] = &io.sound.WAVE4;
    /* FF35 */ memoryAccessors[Specs::Registers::Sound::WAVE5] = &io.sound.WAVE5;
    /* FF36 */ memoryAccessors[Specs::Registers::Sound::WAVE6] = &io.sound.WAVE6;
    /* FF37 */ memoryAccessors[Specs::Registers::Sound::WAVE7] = &io.sound.WAVE7;
    /* FF38 */ memoryAccessors[Specs::Registers::Sound::WAVE8] = &io.sound.WAVE8;
    /* FF39 */ memoryAccessors[Specs::Registers::Sound::WAVE9] = &io.sound.WAVE9;
    /* FF3A */ memoryAccessors[Specs::Registers::Sound::WAVEA] = &io.sound.WAVEA;
    /* FF3B */ memoryAccessors[Specs::Registers::Sound::WAVEB] = &io.sound.WAVEB;
    /* FF3C */ memoryAccessors[Specs::Registers::Sound::WAVEC] = &io.sound.WAVEC;
    /* FF3D */ memoryAccessors[Specs::Registers::Sound::WAVED] = &io.sound.WAVED;
    /* FF3E */ memoryAccessors[Specs::Registers::Sound::WAVEE] = &io.sound.WAVEE;
    /* FF3F */ memoryAccessors[Specs::Registers::Sound::WAVEF] = &io.sound.WAVEF;
    /* FF40 */ memoryAccessors[Specs::Registers::Video::LCDC] = &io.video.LCDC;
    /* FF41 */ memoryAccessors[Specs::Registers::Video::STAT] = {&io.video.STAT, &CpuBus::writeSTAT};
    /* FF42 */ memoryAccessors[Specs::Registers::Video::SCY] = &io.video.SCY;
    /* FF43 */ memoryAccessors[Specs::Registers::Video::SCX] = &io.video.SCX;
    /* FF44 */ memoryAccessors[Specs::Registers::Video::LY] = {&io.video.LY, &CpuBus::writeNop};
    /* FF45 */ memoryAccessors[Specs::Registers::Video::LYC] = &io.video.LYC;
    /* FF46 */ memoryAccessors[Specs::Registers::Video::DMA] = {&io.video.DMA, &CpuBus::writeDMA};
    /* FF47 */ memoryAccessors[Specs::Registers::Video::BGP] = &io.video.BGP;
    /* FF48 */ memoryAccessors[Specs::Registers::Video::OBP0] = &io.video.OBP0;
    /* FF49 */ memoryAccessors[Specs::Registers::Video::OBP1] = &io.video.OBP1;
    /* FF4A */ memoryAccessors[Specs::Registers::Video::WY] = &io.video.WY;
    /* FF4B */ memoryAccessors[Specs::Registers::Video::WX] = &io.video.WX;
    /* FF4C */ memoryAccessors[0xFF4C] = FF;
    /* FF4D */ memoryAccessors[0xFF4D] = FF;
    /* FF4E */ memoryAccessors[0xFF4E] = FF;
    /* FF4F */ memoryAccessors[0xFF4F] = FF;
    /* FF50 */ memoryAccessors[Specs::Registers::Boot::BOOT] = {&io.boot.BOOT, &CpuBus::writeBOOT};
    /* FF51 */ memoryAccessors[0xFF51] = FF;
    /* FF52 */ memoryAccessors[0xFF52] = FF;
    /* FF53 */ memoryAccessors[0xFF53] = FF;
    /* FF54 */ memoryAccessors[0xFF54] = FF;
    /* FF55 */ memoryAccessors[0xFF55] = FF;
    /* FF56 */ memoryAccessors[0xFF56] = FF;
    /* FF57 */ memoryAccessors[0xFF57] = FF;
    /* FF58 */ memoryAccessors[0xFF58] = FF;
    /* FF59 */ memoryAccessors[0xFF59] = FF;
    /* FF5A */ memoryAccessors[0xFF5A] = FF;
    /* FF5B */ memoryAccessors[0xFF5B] = FF;
    /* FF5C */ memoryAccessors[0xFF5C] = FF;
    /* FF5D */ memoryAccessors[0xFF5D] = FF;
    /* FF5E */ memoryAccessors[0xFF5E] = FF;
    /* FF5F */ memoryAccessors[0xFF5F] = FF;
    /* FF60 */ memoryAccessors[0xFF60] = FF;
    /* FF61 */ memoryAccessors[0xFF61] = FF;
    /* FF62 */ memoryAccessors[0xFF62] = FF;
    /* FF63 */ memoryAccessors[0xFF63] = FF;
    /* FF64 */ memoryAccessors[0xFF64] = FF;
    /* FF65 */ memoryAccessors[0xFF65] = FF;
    /* FF66 */ memoryAccessors[0xFF66] = FF;
    /* FF67 */ memoryAccessors[0xFF67] = FF;
    /* FF68 */ memoryAccessors[0xFF68] = FF;
    /* FF69 */ memoryAccessors[0xFF69] = FF;
    /* FF6A */ memoryAccessors[0xFF6A] = FF;
    /* FF6B */ memoryAccessors[0xFF6B] = FF;
    /* FF6C */ memoryAccessors[0xFF6C] = FF;
    /* FF6D */ memoryAccessors[0xFF6D] = FF;
    /* FF6E */ memoryAccessors[0xFF6E] = FF;
    /* FF6F */ memoryAccessors[0xFF6F] = FF;
    /* FF70 */ memoryAccessors[0xFF70] = FF;
    /* FF71 */ memoryAccessors[0xFF71] = FF;
    /* FF72 */ memoryAccessors[0xFF72] = FF;
    /* FF73 */ memoryAccessors[0xFF73] = FF;
    /* FF74 */ memoryAccessors[0xFF74] = FF;
    /* FF75 */ memoryAccessors[0xFF75] = FF;
    /* FF76 */ memoryAccessors[0xFF76] = FF;
    /* FF77 */ memoryAccessors[0xFF77] = FF;
    /* FF78 */ memoryAccessors[0xFF78] = FF;
    /* FF79 */ memoryAccessors[0xFF79] = FF;
    /* FF7A */ memoryAccessors[0xFF7A] = FF;
    /* FF7B */ memoryAccessors[0xFF7B] = FF;
    /* FF7C */ memoryAccessors[0xFF7C] = FF;
    /* FF7D */ memoryAccessors[0xFF7D] = FF;
    /* FF7E */ memoryAccessors[0xFF7E] = FF;
    /* FF7F */ memoryAccessors[0xFF7F] = FF;

    /* 0xFF80 - 0xFFFE */
    for (uint16_t i = Specs::MemoryLayout::HRAM::START; i <= Specs::MemoryLayout::HRAM::END; i++) {
        memoryAccessors[i] = &hram[i - Specs::MemoryLayout::HRAM::START];
    }

    /* FFFF */ memoryAccessors[Specs::MemoryLayout::IE] = &io.interrupts.IE;
}

uint8_t CpuBus::readP1(uint16_t address) const {
    return io.joypad.readP1();
}

void CpuBus::writeP1(uint16_t address, uint8_t value) {
    return io.joypad.writeP1(value);
}

void CpuBus::writeSC(uint16_t address, uint8_t value) {
    return io.serial.writeSC(value);
}

uint8_t CpuBus::readDIV(uint16_t address) const {
    return io.timers.readDIV();
}

void CpuBus::writeDIV(uint16_t address, uint8_t value) {
    io.timers.writeDIV(value);
}

void CpuBus::writeTIMA(uint16_t address, uint8_t value) {
    io.timers.writeTIMA(value);
}

void CpuBus::writeTMA(uint16_t address, uint8_t value) {
    io.timers.writeTMA(value);
}

void CpuBus::writeTAC(uint16_t address, uint8_t value) {
    io.timers.writeTAC(value);
}

void CpuBus::writeIF(uint16_t address, uint8_t value) {
    io.interrupts.writeIF(value);
}

void CpuBus::writeNR10(uint16_t address, uint8_t value) {
    io.sound.writeNR10(value);
}

void CpuBus::writeNR30(uint16_t address, uint8_t value) {
    io.sound.writeNR30(value);
}

void CpuBus::writeNR32(uint16_t address, uint8_t value) {
    io.sound.writeNR32(value);
}

void CpuBus::writeNR41(uint16_t address, uint8_t value) {
    io.sound.writeNR41(value);
}

void CpuBus::writeNR44(uint16_t address, uint8_t value) {
    io.sound.writeNR44(value);
}

void CpuBus::writeNR52(uint16_t address, uint8_t value) {
    io.sound.writeNR52(value);
}

void CpuBus::writeSTAT(uint16_t address, uint8_t value) {
    io.video.writeSTAT(value);
}

void CpuBus::writeDMA(uint16_t address, uint8_t value) {
    io.video.writeDMA(value);
}

void CpuBus::writeBOOT(uint16_t address, uint8_t value) {
    io.boot.writeBOOT(value);
}

uint8_t CpuBus::readFF(uint16_t address) const {
    return 0xFF;
}

void CpuBus::writeNop(uint16_t address, uint8_t value) {
}
