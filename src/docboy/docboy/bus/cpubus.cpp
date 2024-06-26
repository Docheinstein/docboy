#include "docboy/bus/cpubus.h"

#include "docboy/boot/boot.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/memory/hram.h"
#include "docboy/ppu/video.h"
#include "docboy/serial/serial.h"
#include "docboy/sound/sound.h"
#include "docboy/timers/timers.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/bootrom.h"
#endif

#ifdef ENABLE_BOOTROM
CpuBus::CpuBus(BootRom& boot_rom, Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers,
               InterruptsIO& interrupts, SoundIO& sound, VideoIO& video, BootIO& boot) :
    Bus<CpuBus> {},
    boot_rom {boot_rom},
#else
CpuBus::CpuBus(Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers, InterruptsIO& interrupts,
               SoundIO& sound, VideoIO& video, BootIO& boot) :
    Bus<CpuBus> {},
#endif
    hram {hram},
    io {joypad, serial, timers, interrupts, sound, video, boot} {

    const MemoryAccess open_bus_access {&CpuBus::read_ff, &CpuBus::write_nop};

#ifdef ENABLE_BOOTROM
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        memory_accessors[i] = &boot_rom[i];
    }
#endif

    /* FF00 */ memory_accessors[Specs::Registers::Joypad::P1] = {&CpuBus::read_p1, &CpuBus::write_p1};
    /* FF01 */ memory_accessors[Specs::Registers::Serial::SB] = &io.serial.sb;
    /* FF02 */ memory_accessors[Specs::Registers::Serial::SC] = {&io.serial.sc, &CpuBus::write_sc};
    /* FF03 */ memory_accessors[0xFF03] = open_bus_access;
    /* FF04 */ memory_accessors[Specs::Registers::Timers::DIV] = {&CpuBus::read_div, &CpuBus::write_div};
    /* FF05 */ memory_accessors[Specs::Registers::Timers::TIMA] = {&io.timers.tima, &CpuBus::write_tima};
    /* FF06 */ memory_accessors[Specs::Registers::Timers::TMA] = {&io.timers.tma, &CpuBus::write_tma};
    /* FF07 */ memory_accessors[Specs::Registers::Timers::TAC] = {&io.timers.tac, &CpuBus::write_tac};
    /* FF08 */ memory_accessors[0xFF08] = open_bus_access;
    /* FF09 */ memory_accessors[0xFF09] = open_bus_access;
    /* FF0A */ memory_accessors[0xFF0A] = open_bus_access;
    /* FF0B */ memory_accessors[0xFF0B] = open_bus_access;
    /* FF0C */ memory_accessors[0xFF0C] = open_bus_access;
    /* FF0D */ memory_accessors[0xFF0D] = open_bus_access;
    /* FF0E */ memory_accessors[0xFF0E] = open_bus_access;
    /* FF0F */ memory_accessors[Specs::Registers::Interrupts::IF] = {&io.interrupts.IF, &CpuBus::write_if};
    /* FF10 */ memory_accessors[Specs::Registers::Sound::NR10] = {&io.sound.nr10, &CpuBus::write_nr10};
    /* FF11 */ memory_accessors[Specs::Registers::Sound::NR11] = &io.sound.nr11;
    /* FF12 */ memory_accessors[Specs::Registers::Sound::NR12] = &io.sound.nr12;
    /* FF13 */ memory_accessors[Specs::Registers::Sound::NR13] = &io.sound.nr13;
    /* FF14 */ memory_accessors[Specs::Registers::Sound::NR14] = &io.sound.nr14;
    /* FF15 */ memory_accessors[0xFF15] = open_bus_access;
    /* FF16 */ memory_accessors[Specs::Registers::Sound::NR21] = &io.sound.nr21;
    /* FF17 */ memory_accessors[Specs::Registers::Sound::NR22] = &io.sound.nr22;
    /* FF18 */ memory_accessors[Specs::Registers::Sound::NR23] = &io.sound.nr23;
    /* FF19 */ memory_accessors[Specs::Registers::Sound::NR24] = &io.sound.nr24;
    /* FF1A */ memory_accessors[Specs::Registers::Sound::NR30] = {&io.sound.nr30, &CpuBus::write_nr30};
    /* FF1B */ memory_accessors[Specs::Registers::Sound::NR31] = &io.sound.nr31;
    /* FF1C */ memory_accessors[Specs::Registers::Sound::NR32] = {&io.sound.nr32, &CpuBus::write_nr32};
    /* FF1D */ memory_accessors[Specs::Registers::Sound::NR33] = &io.sound.nr33;
    /* FF1E */ memory_accessors[Specs::Registers::Sound::NR34] = &io.sound.nr34;
    /* FF1F */ memory_accessors[0xFF1F] = open_bus_access;
    /* FF20 */ memory_accessors[Specs::Registers::Sound::NR41] = {&io.sound.nr41, &CpuBus::write_nr41};
    /* FF21 */ memory_accessors[Specs::Registers::Sound::NR42] = &io.sound.nr42;
    /* FF22 */ memory_accessors[Specs::Registers::Sound::NR43] = &io.sound.nr43;
    /* FF23 */ memory_accessors[Specs::Registers::Sound::NR44] = {&io.sound.nr44, &CpuBus::write_nr44};
    /* FF24 */ memory_accessors[Specs::Registers::Sound::NR50] = &io.sound.nr50;
    /* FF25 */ memory_accessors[Specs::Registers::Sound::NR51] = &io.sound.nr51;
    /* FF26 */ memory_accessors[Specs::Registers::Sound::NR52] = {&io.sound.nr52, &CpuBus::write_nr52};
    /* FF27 */ memory_accessors[0xFF27] = open_bus_access;
    /* FF28 */ memory_accessors[0xFF28] = open_bus_access;
    /* FF29 */ memory_accessors[0xFF29] = open_bus_access;
    /* FF2A */ memory_accessors[0xFF2A] = open_bus_access;
    /* FF2B */ memory_accessors[0xFF2B] = open_bus_access;
    /* FF2C */ memory_accessors[0xFF2C] = open_bus_access;
    /* FF2D */ memory_accessors[0xFF2D] = open_bus_access;
    /* FF2E */ memory_accessors[0xFF2E] = open_bus_access;
    /* FF2F */ memory_accessors[0xFF2F] = open_bus_access;
    /* FF30 */ memory_accessors[Specs::Registers::Sound::WAVE0] = &io.sound.wave0;
    /* FF31 */ memory_accessors[Specs::Registers::Sound::WAVE1] = &io.sound.wave1;
    /* FF32 */ memory_accessors[Specs::Registers::Sound::WAVE2] = &io.sound.wave2;
    /* FF33 */ memory_accessors[Specs::Registers::Sound::WAVE3] = &io.sound.wave3;
    /* FF34 */ memory_accessors[Specs::Registers::Sound::WAVE4] = &io.sound.wave4;
    /* FF35 */ memory_accessors[Specs::Registers::Sound::WAVE5] = &io.sound.wave5;
    /* FF36 */ memory_accessors[Specs::Registers::Sound::WAVE6] = &io.sound.wave6;
    /* FF37 */ memory_accessors[Specs::Registers::Sound::WAVE7] = &io.sound.wave7;
    /* FF38 */ memory_accessors[Specs::Registers::Sound::WAVE8] = &io.sound.wave8;
    /* FF39 */ memory_accessors[Specs::Registers::Sound::WAVE9] = &io.sound.wave9;
    /* FF3A */ memory_accessors[Specs::Registers::Sound::WAVEA] = &io.sound.waveA;
    /* FF3B */ memory_accessors[Specs::Registers::Sound::WAVEB] = &io.sound.waveB;
    /* FF3C */ memory_accessors[Specs::Registers::Sound::WAVEC] = &io.sound.waveC;
    /* FF3D */ memory_accessors[Specs::Registers::Sound::WAVED] = &io.sound.waveD;
    /* FF3E */ memory_accessors[Specs::Registers::Sound::WAVEE] = &io.sound.waveE;
    /* FF3F */ memory_accessors[Specs::Registers::Sound::WAVEF] = &io.sound.waveF;
    /* FF40 */ memory_accessors[Specs::Registers::Video::LCDC] = &io.video.lcdc;
    /* FF41 */ memory_accessors[Specs::Registers::Video::STAT] = {&io.video.stat, &CpuBus::write_stat};
    /* FF42 */ memory_accessors[Specs::Registers::Video::SCY] = &io.video.scy;
    /* FF43 */ memory_accessors[Specs::Registers::Video::SCX] = &io.video.scx;
    /* FF44 */ memory_accessors[Specs::Registers::Video::LY] = {&io.video.ly, &CpuBus::write_nop};
    /* FF45 */ memory_accessors[Specs::Registers::Video::LYC] = &io.video.lyc;
    /* FF46 */ memory_accessors[Specs::Registers::Video::DMA] = {&io.video.dma, &CpuBus::write_dma};
    /* FF47 */ memory_accessors[Specs::Registers::Video::BGP] = &io.video.bgp;
    /* FF48 */ memory_accessors[Specs::Registers::Video::OBP0] = &io.video.obp0;
    /* FF49 */ memory_accessors[Specs::Registers::Video::OBP1] = &io.video.obp1;
    /* FF4A */ memory_accessors[Specs::Registers::Video::WY] = &io.video.wy;
    /* FF4B */ memory_accessors[Specs::Registers::Video::WX] = &io.video.wx;
    /* FF4C */ memory_accessors[0xFF4C] = open_bus_access;
    /* FF4D */ memory_accessors[0xFF4D] = open_bus_access;
    /* FF4E */ memory_accessors[0xFF4E] = open_bus_access;
    /* FF4F */ memory_accessors[0xFF4F] = open_bus_access;
    /* FF50 */ memory_accessors[Specs::Registers::Boot::BOOT] = {&io.boot.boot, &CpuBus::write_boot};
    /* FF51 */ memory_accessors[0xFF51] = open_bus_access;
    /* FF52 */ memory_accessors[0xFF52] = open_bus_access;
    /* FF53 */ memory_accessors[0xFF53] = open_bus_access;
    /* FF54 */ memory_accessors[0xFF54] = open_bus_access;
    /* FF55 */ memory_accessors[0xFF55] = open_bus_access;
    /* FF56 */ memory_accessors[0xFF56] = open_bus_access;
    /* FF57 */ memory_accessors[0xFF57] = open_bus_access;
    /* FF58 */ memory_accessors[0xFF58] = open_bus_access;
    /* FF59 */ memory_accessors[0xFF59] = open_bus_access;
    /* FF5A */ memory_accessors[0xFF5A] = open_bus_access;
    /* FF5B */ memory_accessors[0xFF5B] = open_bus_access;
    /* FF5C */ memory_accessors[0xFF5C] = open_bus_access;
    /* FF5D */ memory_accessors[0xFF5D] = open_bus_access;
    /* FF5E */ memory_accessors[0xFF5E] = open_bus_access;
    /* FF5F */ memory_accessors[0xFF5F] = open_bus_access;
    /* FF60 */ memory_accessors[0xFF60] = open_bus_access;
    /* FF61 */ memory_accessors[0xFF61] = open_bus_access;
    /* FF62 */ memory_accessors[0xFF62] = open_bus_access;
    /* FF63 */ memory_accessors[0xFF63] = open_bus_access;
    /* FF64 */ memory_accessors[0xFF64] = open_bus_access;
    /* FF65 */ memory_accessors[0xFF65] = open_bus_access;
    /* FF66 */ memory_accessors[0xFF66] = open_bus_access;
    /* FF67 */ memory_accessors[0xFF67] = open_bus_access;
    /* FF68 */ memory_accessors[0xFF68] = open_bus_access;
    /* FF69 */ memory_accessors[0xFF69] = open_bus_access;
    /* FF6A */ memory_accessors[0xFF6A] = open_bus_access;
    /* FF6B */ memory_accessors[0xFF6B] = open_bus_access;
    /* FF6C */ memory_accessors[0xFF6C] = open_bus_access;
    /* FF6D */ memory_accessors[0xFF6D] = open_bus_access;
    /* FF6E */ memory_accessors[0xFF6E] = open_bus_access;
    /* FF6F */ memory_accessors[0xFF6F] = open_bus_access;
    /* FF70 */ memory_accessors[0xFF70] = open_bus_access;
    /* FF71 */ memory_accessors[0xFF71] = open_bus_access;
    /* FF72 */ memory_accessors[0xFF72] = open_bus_access;
    /* FF73 */ memory_accessors[0xFF73] = open_bus_access;
    /* FF74 */ memory_accessors[0xFF74] = open_bus_access;
    /* FF75 */ memory_accessors[0xFF75] = open_bus_access;
    /* FF76 */ memory_accessors[0xFF76] = open_bus_access;
    /* FF77 */ memory_accessors[0xFF77] = open_bus_access;
    /* FF78 */ memory_accessors[0xFF78] = open_bus_access;
    /* FF79 */ memory_accessors[0xFF79] = open_bus_access;
    /* FF7A */ memory_accessors[0xFF7A] = open_bus_access;
    /* FF7B */ memory_accessors[0xFF7B] = open_bus_access;
    /* FF7C */ memory_accessors[0xFF7C] = open_bus_access;
    /* FF7D */ memory_accessors[0xFF7D] = open_bus_access;
    /* FF7E */ memory_accessors[0xFF7E] = open_bus_access;
    /* FF7F */ memory_accessors[0xFF7F] = open_bus_access;

    /* 0xFF80 - 0xFFFE */
    for (uint16_t i = Specs::MemoryLayout::HRAM::START; i <= Specs::MemoryLayout::HRAM::END; i++) {
        memory_accessors[i] = &hram[i - Specs::MemoryLayout::HRAM::START];
    }

    /* FFFF */ memory_accessors[Specs::MemoryLayout::IE] = &io.interrupts.IE;
}

uint8_t CpuBus::read_p1(uint16_t address) const {
    return io.joypad.read_p1();
}

void CpuBus::write_p1(uint16_t address, uint8_t value) {
    return io.joypad.write_p1(value);
}

void CpuBus::write_sc(uint16_t address, uint8_t value) {
    return io.serial.write_SC(value);
}

uint8_t CpuBus::read_div(uint16_t address) const {
    return io.timers.read_div();
}

void CpuBus::write_div(uint16_t address, uint8_t value) {
    io.timers.write_div(value);
}

void CpuBus::write_tima(uint16_t address, uint8_t value) {
    io.timers.write_tima(value);
}

void CpuBus::write_tma(uint16_t address, uint8_t value) {
    io.timers.write_tma(value);
}

void CpuBus::write_tac(uint16_t address, uint8_t value) {
    io.timers.write_tac(value);
}

void CpuBus::write_if(uint16_t address, uint8_t value) {
    io.interrupts.write_IF(value);
}

void CpuBus::write_nr10(uint16_t address, uint8_t value) {
    io.sound.write_nr10(value);
}

void CpuBus::write_nr30(uint16_t address, uint8_t value) {
    io.sound.write_nr30(value);
}

void CpuBus::write_nr32(uint16_t address, uint8_t value) {
    io.sound.write_nr32(value);
}

void CpuBus::write_nr41(uint16_t address, uint8_t value) {
    io.sound.write_nr41(value);
}

void CpuBus::write_nr44(uint16_t address, uint8_t value) {
    io.sound.write_nr44(value);
}

void CpuBus::write_nr52(uint16_t address, uint8_t value) {
    io.sound.write_nr52(value);
}

void CpuBus::write_stat(uint16_t address, uint8_t value) {
    io.video.write_stat(value);
}

void CpuBus::write_dma(uint16_t address, uint8_t value) {
    io.video.write_dma(value);
}

void CpuBus::write_boot(uint16_t address, uint8_t value) {
    io.boot.write_boot(value);
}

uint8_t CpuBus::read_ff(uint16_t address) const {
    return 0xFF;
}

void CpuBus::write_nop(uint16_t address, uint8_t value) {
}
