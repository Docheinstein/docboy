#include "docboy/bus/cpubus.h"

#include "docboy/audio/apu.h"
#include "docboy/boot/boot.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/memory/hram.h"
#include "docboy/ppu/ppu.h"
#include "docboy/serial/serial.h"
#include "docboy/timers/timers.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/bootrom.h"
#endif

#ifdef ENABLE_BOOTROM
CpuBus::CpuBus(BootRom& boot_rom, Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers,
               InterruptsIO& interrupts, Apu& apu, Ppu& ppu, BootIO& boot) :
    Bus<CpuBus> {},
    boot_rom {boot_rom},
#else
CpuBus::CpuBus(Hram& hram, JoypadIO& joypad, SerialIO& serial, TimersIO& timers, InterruptsIO& interrupts, Apu& apu,
               Ppu& ppu, BootIO& boot) :
    Bus {},
#endif
    hram {hram},
    joypad {joypad},
    serial {serial},
    timers {timers},
    interrupts {interrupts},
    boot {boot},
    apu {apu},
    ppu {ppu} {

    const NonTrivialReadFunctor read_ff = {[](void*, uint16_t) -> uint8_t {
                                               return 0xFF;
                                           },
                                           nullptr};
    const NonTrivialWriteFunctor write_nop = {[](void*, uint16_t, uint8_t) {
                                              },
                                              nullptr};
    const MemoryAccess open_bus_access {read_ff, write_nop};

#ifdef ENABLE_BOOTROM
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        memory_accessors[i] = &boot_rom[i];
    }
#endif

    /* FF00 */ memory_accessors[Specs::Registers::Joypad::P1] = {
        NonTrivialRead<JoypadIO, &JoypadIO::read_p1> {&joypad},
        NonTrivialWrite<JoypadIO, &JoypadIO::write_p1> {&joypad}};
    /* FF01 */ memory_accessors[Specs::Registers::Serial::SB] = &serial.sb;
    /* FF02 */ memory_accessors[Specs::Registers::Serial::SC] = {
        &serial.sc, NonTrivialWrite<SerialIO, &SerialIO::write_sc> {&serial}};
    /* FF03 */ memory_accessors[0xFF03] = open_bus_access;
    /* FF04 */ memory_accessors[Specs::Registers::Timers::DIV] = {
        NonTrivialRead<TimersIO, &TimersIO::read_div> {&timers},
        NonTrivialWrite<TimersIO, &TimersIO::write_div> {&timers}};
    /* FF05 */ memory_accessors[Specs::Registers::Timers::TIMA] = {
        &timers.tima, NonTrivialWrite<TimersIO, &TimersIO::write_tima> {&timers}};
    /* FF06 */ memory_accessors[Specs::Registers::Timers::TMA] = {
        &timers.tma, NonTrivialWrite<TimersIO, &TimersIO::write_tma> {&timers}};
    /* FF07 */ memory_accessors[Specs::Registers::Timers::TAC] = {
        &timers.tac, NonTrivialWrite<TimersIO, &TimersIO::write_tac> {&timers}};
    /* FF08 */ memory_accessors[0xFF08] = open_bus_access;
    /* FF09 */ memory_accessors[0xFF09] = open_bus_access;
    /* FF0A */ memory_accessors[0xFF0A] = open_bus_access;
    /* FF0B */ memory_accessors[0xFF0B] = open_bus_access;
    /* FF0C */ memory_accessors[0xFF0C] = open_bus_access;
    /* FF0D */ memory_accessors[0xFF0D] = open_bus_access;
    /* FF0E */ memory_accessors[0xFF0E] = open_bus_access;
    /* FF0F */ memory_accessors[Specs::Registers::Interrupts::IF] = {
        &interrupts.IF, NonTrivialWrite<InterruptsIO, &InterruptsIO::write_IF> {&interrupts}};
    /* FF10 */ memory_accessors[Specs::Registers::Sound::NR10] = {&apu.nr10,
                                                                  NonTrivialWrite<Apu, &Apu::write_nr10> {&apu}};
    /* FF11 */ memory_accessors[Specs::Registers::Sound::NR11] = &apu.nr11;
    /* FF12 */ memory_accessors[Specs::Registers::Sound::NR12] = &apu.nr12;
    /* FF13 */ memory_accessors[Specs::Registers::Sound::NR13] = &apu.nr13;
    /* FF14 */ memory_accessors[Specs::Registers::Sound::NR14] = &apu.nr14;
    /* FF15 */ memory_accessors[0xFF15] = open_bus_access;
    /* FF16 */ memory_accessors[Specs::Registers::Sound::NR21] = CompositeMemoryAccess {&apu.nr21};
    /* FF17 */ memory_accessors[Specs::Registers::Sound::NR22] = CompositeMemoryAccess {&apu.nr22};
    /* FF18 */ memory_accessors[Specs::Registers::Sound::NR23] = &apu.nr23;
    /* FF19 */ memory_accessors[Specs::Registers::Sound::NR24] = CompositeMemoryAccess {&apu.nr24};
    /* FF1A */ memory_accessors[Specs::Registers::Sound::NR30] = {&apu.nr30,
                                                                  NonTrivialWrite<Apu, &Apu::write_nr30> {&apu}};
    /* FF1B */ memory_accessors[Specs::Registers::Sound::NR31] = &apu.nr31;
    /* FF1C */ memory_accessors[Specs::Registers::Sound::NR32] = {&apu.nr32,
                                                                  NonTrivialWrite<Apu, &Apu::write_nr32> {&apu}};
    /* FF1D */ memory_accessors[Specs::Registers::Sound::NR33] = &apu.nr33;
    /* FF1E */ memory_accessors[Specs::Registers::Sound::NR34] = &apu.nr34;
    /* FF1F */ memory_accessors[0xFF1F] = open_bus_access;
    /* FF20 */ memory_accessors[Specs::Registers::Sound::NR41] = {&apu.nr41,
                                                                  NonTrivialWrite<Apu, &Apu::write_nr41> {&apu}};
    /* FF21 */ memory_accessors[Specs::Registers::Sound::NR42] = &apu.nr42;
    /* FF22 */ memory_accessors[Specs::Registers::Sound::NR43] = &apu.nr43;
    /* FF23 */ memory_accessors[Specs::Registers::Sound::NR44] = {&apu.nr44,
                                                                  NonTrivialWrite<Apu, &Apu::write_nr44> {&apu}};
    /* FF24 */ memory_accessors[Specs::Registers::Sound::NR50] = &apu.nr50;
    /* FF25 */ memory_accessors[Specs::Registers::Sound::NR51] = &apu.nr51;
    /* FF26 */ memory_accessors[Specs::Registers::Sound::NR52] = CompositeMemoryAccess {&apu.nr52};
    /* FF27 */ memory_accessors[0xFF27] = open_bus_access;
    /* FF28 */ memory_accessors[0xFF28] = open_bus_access;
    /* FF29 */ memory_accessors[0xFF29] = open_bus_access;
    /* FF2A */ memory_accessors[0xFF2A] = open_bus_access;
    /* FF2B */ memory_accessors[0xFF2B] = open_bus_access;
    /* FF2C */ memory_accessors[0xFF2C] = open_bus_access;
    /* FF2D */ memory_accessors[0xFF2D] = open_bus_access;
    /* FF2E */ memory_accessors[0xFF2E] = open_bus_access;
    /* FF2F */ memory_accessors[0xFF2F] = open_bus_access;
    /* FF30 */ memory_accessors[Specs::Registers::Sound::WAVE0] = &apu.wave0;
    /* FF31 */ memory_accessors[Specs::Registers::Sound::WAVE1] = &apu.wave1;
    /* FF32 */ memory_accessors[Specs::Registers::Sound::WAVE2] = &apu.wave2;
    /* FF33 */ memory_accessors[Specs::Registers::Sound::WAVE3] = &apu.wave3;
    /* FF34 */ memory_accessors[Specs::Registers::Sound::WAVE4] = &apu.wave4;
    /* FF35 */ memory_accessors[Specs::Registers::Sound::WAVE5] = &apu.wave5;
    /* FF36 */ memory_accessors[Specs::Registers::Sound::WAVE6] = &apu.wave6;
    /* FF37 */ memory_accessors[Specs::Registers::Sound::WAVE7] = &apu.wave7;
    /* FF38 */ memory_accessors[Specs::Registers::Sound::WAVE8] = &apu.wave8;
    /* FF39 */ memory_accessors[Specs::Registers::Sound::WAVE9] = &apu.wave9;
    /* FF3A */ memory_accessors[Specs::Registers::Sound::WAVEA] = &apu.waveA;
    /* FF3B */ memory_accessors[Specs::Registers::Sound::WAVEB] = &apu.waveB;
    /* FF3C */ memory_accessors[Specs::Registers::Sound::WAVEC] = &apu.waveC;
    /* FF3D */ memory_accessors[Specs::Registers::Sound::WAVED] = &apu.waveD;
    /* FF3E */ memory_accessors[Specs::Registers::Sound::WAVEE] = &apu.waveE;
    /* FF3F */ memory_accessors[Specs::Registers::Sound::WAVEF] = &apu.waveF;
    /* FF40 */ memory_accessors[Specs::Registers::Video::LCDC] = CompositeMemoryAccess {&ppu.lcdc};
    /* FF41 */ memory_accessors[Specs::Registers::Video::STAT] = CompositeMemoryAccess {&ppu.stat};
    /* FF42 */ memory_accessors[Specs::Registers::Video::SCY] = &ppu.scy;
    /* FF43 */ memory_accessors[Specs::Registers::Video::SCX] = &ppu.scx;
    /* FF44 */ memory_accessors[Specs::Registers::Video::LY] = {&ppu.ly, write_nop};
    /* FF45 */ memory_accessors[Specs::Registers::Video::LYC] = &ppu.lyc;
    /* FF46 */ memory_accessors[Specs::Registers::Video::DMA] = {&ppu.dma,
                                                                 NonTrivialWrite<Ppu, &Ppu::write_dma> {&ppu}};
    /* FF47 */ memory_accessors[Specs::Registers::Video::BGP] = &ppu.bgp;
    /* FF48 */ memory_accessors[Specs::Registers::Video::OBP0] = &ppu.obp0;
    /* FF49 */ memory_accessors[Specs::Registers::Video::OBP1] = &ppu.obp1;
    /* FF4A */ memory_accessors[Specs::Registers::Video::WY] = &ppu.wy;
    /* FF4B */ memory_accessors[Specs::Registers::Video::WX] = &ppu.wx;
    /* FF4C */ memory_accessors[0xFF4C] = open_bus_access;
    /* FF4D */ memory_accessors[0xFF4D] = open_bus_access;
    /* FF4E */ memory_accessors[0xFF4E] = open_bus_access;
    /* FF4F */ memory_accessors[0xFF4F] = open_bus_access;
    /* FF50 */ memory_accessors[Specs::Registers::Boot::BOOT] = {&boot.boot,
                                                                 NonTrivialWrite<BootIO, &BootIO::write_boot> {&boot}};
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

    /* FFFF */ memory_accessors[Specs::MemoryLayout::IE] = &interrupts.IE;
}
