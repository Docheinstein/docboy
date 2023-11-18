#include "mmu.h"
#include "docboy/boot/boot.h"
#include "docboy/cartridge/slot.h"
#include "docboy/dma/dma.h"
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

Mmu::MemoryAccess::MemoryAccess(byte* rw) {
    read.trivial = write.trivial = rw;
}

Mmu::MemoryAccess::MemoryAccess(byte* r, byte* w) {
    read.trivial = r;
    write.trivial = w;
}

Mmu::MemoryAccess::MemoryAccess(Mmu::NonTrivialMemoryRead r, byte* w) {
    read.nonTrivial = r;
    read.trivial = w;
}

Mmu::MemoryAccess::MemoryAccess(byte* r, Mmu::NonTrivialMemoryWrite w) {
    read.trivial = r;
    write.nonTrivial = w;
}

Mmu::MemoryAccess::MemoryAccess(Mmu::NonTrivialMemoryRead r, Mmu::NonTrivialMemoryWrite w) {
    read.nonTrivial = r;
    write.nonTrivial = w;
}

Mmu::Mmu(IF_BOOTROM(BootRom& bootRom COMMA) CartridgeSlot& cartridgeSlot, Vram& vram, Wram1& wram1, Wram2& wram2,
         Oam& oam, Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers, InterruptsIO& interrupts,
         SoundIO& sound, VideoIO& video, BootIO& boot, Dma& dma) :
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
    dma(dma) {

    const MemoryAccess FF {&Mmu::readFF, &Mmu::writeNop};

    /* 0x0000 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::ROM0::START; i <= Specs::MemoryLayout::ROM1::END; i++) {
        memoryAccessors[i] = {&Mmu::readCartridgeRom, &Mmu::writeCartridgeRom};
    }

    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memoryAccessors[i] = {&Mmu::readVram, &Mmu::writeVram};
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        memoryAccessors[i] = {&Mmu::readCartridgeRam, &Mmu::writeCartridgeRam};
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
        memoryAccessors[i] = &wram1[i - Specs::MemoryLayout::ECHO_RAM::START];
    }

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memoryAccessors[i] = {&Mmu::readOam, &Mmu::writeOam};
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memoryAccessors[i] = FF;
    }

    /* FF00 */ memoryAccessors[Specs::Registers::Joypad::P1] = {&Mmu::readP1, &Mmu::writeP1};
    /* FF01 */ memoryAccessors[Specs::Registers::Serial::SB] = &serial.SB;
    /* FF02 */ memoryAccessors[Specs::Registers::Serial::SC] = {&serial.SC, &Mmu::writeSC};
    /* FF03 */ memoryAccessors[0xFF03] = FF;
    /* FF04 */ memoryAccessors[Specs::Registers::Timers::DIV] = {&Mmu::readDIV, &Mmu::writeDIV};
    /* FF05 */ memoryAccessors[Specs::Registers::Timers::TIMA] = {&timers.TIMA, &Mmu::writeTIMA};
    /* FF06 */ memoryAccessors[Specs::Registers::Timers::TMA] = {&timers.TMA, &Mmu::writeTMA};
    /* FF07 */ memoryAccessors[Specs::Registers::Timers::TAC] = {&timers.TAC, &Mmu::writeTAC};
    /* FF08 */ memoryAccessors[0xFF08] = FF;
    /* FF09 */ memoryAccessors[0xFF09] = FF;
    /* FF0A */ memoryAccessors[0xFF0A] = FF;
    /* FF0B */ memoryAccessors[0xFF0B] = FF;
    /* FF0C */ memoryAccessors[0xFF0C] = FF;
    /* FF0D */ memoryAccessors[0xFF0D] = FF;
    /* FF0E */ memoryAccessors[0xFF0E] = FF;
    /* FF0F */ memoryAccessors[Specs::Registers::Interrupts::IF] = {&interrupts.IF, &Mmu::writeIF};
    /* FF10 */ memoryAccessors[Specs::Registers::Sound::NR10] = {&sound.NR10, &Mmu::writeNR10};
    /* FF11 */ memoryAccessors[Specs::Registers::Sound::NR11] = &sound.NR11;
    /* FF12 */ memoryAccessors[Specs::Registers::Sound::NR12] = &sound.NR12;
    /* FF13 */ memoryAccessors[Specs::Registers::Sound::NR13] = &sound.NR13;
    /* FF14 */ memoryAccessors[Specs::Registers::Sound::NR14] = &sound.NR14;
    /* FF15 */ memoryAccessors[0xFF15] = FF;
    /* FF16 */ memoryAccessors[Specs::Registers::Sound::NR21] = &sound.NR21;
    /* FF17 */ memoryAccessors[Specs::Registers::Sound::NR22] = &sound.NR22;
    /* FF18 */ memoryAccessors[Specs::Registers::Sound::NR23] = &sound.NR23;
    /* FF19 */ memoryAccessors[Specs::Registers::Sound::NR24] = &sound.NR24;
    /* FF1A */ memoryAccessors[Specs::Registers::Sound::NR30] = {&sound.NR30, &Mmu::writeNR30};
    /* FF1B */ memoryAccessors[Specs::Registers::Sound::NR31] = &sound.NR31;
    /* FF1C */ memoryAccessors[Specs::Registers::Sound::NR32] = {&sound.NR32, &Mmu::writeNR32};
    /* FF1D */ memoryAccessors[Specs::Registers::Sound::NR33] = &sound.NR33;
    /* FF1E */ memoryAccessors[Specs::Registers::Sound::NR34] = &sound.NR34;
    /* FF1F */ memoryAccessors[0xFF1F] = FF;
    /* FF20 */ memoryAccessors[Specs::Registers::Sound::NR41] = {&sound.NR41, &Mmu::writeNR41};
    /* FF21 */ memoryAccessors[Specs::Registers::Sound::NR42] = &sound.NR42;
    /* FF22 */ memoryAccessors[Specs::Registers::Sound::NR43] = &sound.NR43;
    /* FF23 */ memoryAccessors[Specs::Registers::Sound::NR44] = {&sound.NR44, &Mmu::writeNR44};
    /* FF24 */ memoryAccessors[Specs::Registers::Sound::NR50] = &sound.NR50;
    /* FF25 */ memoryAccessors[Specs::Registers::Sound::NR51] = &sound.NR51;
    /* FF26 */ memoryAccessors[Specs::Registers::Sound::NR52] = {&sound.NR52, &Mmu::writeNR52};
    /* FF27 */ memoryAccessors[0xFF27] = FF;
    /* FF28 */ memoryAccessors[0xFF28] = FF;
    /* FF29 */ memoryAccessors[0xFF29] = FF;
    /* FF2A */ memoryAccessors[0xFF2A] = FF;
    /* FF2B */ memoryAccessors[0xFF2B] = FF;
    /* FF2C */ memoryAccessors[0xFF2C] = FF;
    /* FF2D */ memoryAccessors[0xFF2D] = FF;
    /* FF2E */ memoryAccessors[0xFF2E] = FF;
    /* FF2F */ memoryAccessors[0xFF2F] = FF;
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
    /* FF41 */ memoryAccessors[Specs::Registers::Video::STAT] = {&video.STAT, &Mmu::writeSTAT};
    /* FF42 */ memoryAccessors[Specs::Registers::Video::SCY] = &video.SCY;
    /* FF43 */ memoryAccessors[Specs::Registers::Video::SCX] = &video.SCX;
    /* FF44 */ memoryAccessors[Specs::Registers::Video::LY] = {&video.LY, &Mmu::writeNop};
    /* FF45 */ memoryAccessors[Specs::Registers::Video::LYC] = {&video.LYC, &Mmu::writeLYC};
    /* FF46 */ memoryAccessors[Specs::Registers::Video::DMA] = {&video.DMA, &Mmu::writeDMA};
    /* FF47 */ memoryAccessors[Specs::Registers::Video::BGP] = &video.BGP;
    /* FF48 */ memoryAccessors[Specs::Registers::Video::OBP0] = &video.OBP0;
    /* FF49 */ memoryAccessors[Specs::Registers::Video::OBP1] = &video.OBP1;
    /* FF4A */ memoryAccessors[Specs::Registers::Video::WY] = &video.WY;
    /* FF4B */ memoryAccessors[Specs::Registers::Video::WX] = &video.WX;
    /* FF4C */ memoryAccessors[0xFF4C] = FF;
    /* FF4D */ memoryAccessors[0xFF4D] = FF;
    /* FF4E */ memoryAccessors[0xFF4E] = FF;
    /* FF4F */ memoryAccessors[0xFF4F] = FF;
    /* FF50 */ memoryAccessors[Specs::Registers::Boot::BOOT] = {&boot.BOOT, &Mmu::writeBOOT};
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

    /* FFFF */ memoryAccessors[Specs::MemoryLayout::IE] = &interrupts.IE;
}

void Mmu::requestRead(uint16_t address, uint8_t& dest) {
    readRequest.pending = true;
    readRequest.address = address;
    readRequest.dest = &dest;
}

void Mmu::requestWrite(uint16_t address, uint8_t value) {
    writeRequest.pending = true;
    writeRequest.address = address;
    writeRequest.value = value;
}

void Mmu::flushReadRequest() {
    if (readRequest.pending) {
        readRequest.pending = false;
        *readRequest.dest = read(readRequest.address);
    }
}

void Mmu::flushWriteRequest() {
    if (writeRequest.pending) {
        writeRequest.pending = false;
        write(writeRequest.address, writeRequest.value);
    }
}

uint8_t Mmu::read(uint16_t address) const {
    const MemoryAccess::Read& memRead = memoryAccessors[address].read;
    return memRead.trivial ? static_cast<uint8_t>(*memRead.trivial) : (this->*(memRead.nonTrivial))(address);
}

void Mmu::write(uint16_t address, uint8_t value) {
    const MemoryAccess::Write& memoryAccess = memoryAccessors[address].write;
    if (memoryAccess.trivial) {
        *memoryAccess.trivial = value;
    } else {
        (this->*(memoryAccess.nonTrivial))(address, value);
    }
}

uint8_t Mmu::readCartridgeRom(uint16_t address) const {
#ifdef ENABLE_BOOTROM
    if (test_bit<Specs::Bits::Boot::BOOT_ENABLE>(boot.BOOT) == 0 && address <= Specs::MemoryLayout::BOOTROM::END) {
        return bootRom[address];
    }
#endif

    return cartridgeSlot.readRom(address);
}

void Mmu::writeCartridgeRom(uint16_t address, uint8_t value) {
    return cartridgeSlot.writeRom(address, value);
}

uint8_t Mmu::readCartridgeRam(uint16_t address) const {
    return cartridgeSlot.readRam(address);
}

void Mmu::writeCartridgeRam(uint16_t address, uint8_t value) {
    return cartridgeSlot.writeRam(address, value);
}

uint8_t Mmu::readVram(uint16_t address) const {
    // VRAM is blocked during Pixel Transfer
    if (keep_bits<2>(video.STAT) != Specs::Ppu::Modes::PIXEL_TRANSFER)
        return vram[address - Specs::MemoryLayout::VRAM::START];
    return 0xFF;
}

void Mmu::writeVram(uint16_t address, uint8_t value) {
    // VRAM is blocked during Pixel Transfer
    if (keep_bits<2>(video.STAT) != Specs::Ppu::Modes::PIXEL_TRANSFER)
        vram[address - Specs::MemoryLayout::VRAM::START] = value;
}

uint8_t Mmu::readOam(uint16_t address) const {
    // OAM is blocked:
    // * during OAM Scan and Pixel Transfer
    // * during DMA transfer
    if (!dma.isTransferring() && keep_bits<2>(video.STAT) < Specs::Ppu::Modes::OAM_SCAN)
        return oam[address - Specs::MemoryLayout::OAM::START];
    return 0xFF;
}

void Mmu::writeOam(uint16_t address, uint8_t value) {
    // OAM is blocked:
    // * during OAM Scan and Pixel Transfer
    // * during DMA transfer
    if (!dma.isTransferring() && keep_bits<2>(video.STAT) < Specs::Ppu::Modes::OAM_SCAN)
        oam[address - Specs::MemoryLayout::OAM::START] = value;
}

uint8_t Mmu::readP1(uint16_t address) const {
    return joypad.readP1();
}

void Mmu::writeP1(uint16_t address, uint8_t value) {
    return joypad.writeP1(value);
}

void Mmu::writeSC(uint16_t address, uint8_t value) {
    return serial.writeSC(value);
}

uint8_t Mmu::readDIV(uint16_t address) const {
    return timers.readDIV();
}

void Mmu::writeDIV(uint16_t address, uint8_t value) {
    timers.writeDIV(value);
}

void Mmu::writeTIMA(uint16_t address, uint8_t value) {
    timers.writeTIMA(value);
}

void Mmu::writeTMA(uint16_t address, uint8_t value) {
    timers.writeTMA(value);
}

void Mmu::writeTAC(uint16_t address, uint8_t value) {
    timers.writeTAC(value);
}

void Mmu::writeIF(uint16_t address, uint8_t value) {
    interrupts.writeIF(value);
}

void Mmu::writeNR10(uint16_t address, uint8_t value) {
    sound.writeNR10(value);
}

void Mmu::writeNR30(uint16_t address, uint8_t value) {
    sound.writeNR30(value);
}

void Mmu::writeNR32(uint16_t address, uint8_t value) {
    sound.writeNR32(value);
}

void Mmu::writeNR41(uint16_t address, uint8_t value) {
    sound.writeNR41(value);
}

void Mmu::writeNR44(uint16_t address, uint8_t value) {
    sound.writeNR44(value);
}

void Mmu::writeNR52(uint16_t address, uint8_t value) {
    sound.writeNR52(value);
}

void Mmu::writeSTAT(uint16_t address, uint8_t value) {
    video.writeSTAT(value);
}

void Mmu::writeDMA(uint16_t address, uint8_t value) {
    video.writeDMA(value);
}

void Mmu::writeLYC(uint16_t address, uint8_t value) {
    video.writeLYC(value);
}

void Mmu::writeBOOT(uint16_t address, uint8_t value) {
    boot.writeBOOT(value);
}

uint8_t Mmu::readFF(uint16_t address) const {
    return 0xFF;
}

void Mmu::writeNop(uint16_t address, uint8_t value) {
}
