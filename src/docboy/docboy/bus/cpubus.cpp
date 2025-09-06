#include "docboy/bus/cpubus.h"

#include "docboy/audio/apu.h"
#include "docboy/boot/boot.h"
#include "docboy/dma/dma.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/memory/hram.h"
#include "docboy/ppu/ppu.h"
#include "docboy/serial/serial.h"
#include "docboy/timers/timers.h"

#ifdef ENABLE_CGB
#include "docboy/banks/vrambankcontroller.h"
#include "docboy/banks/wrambankcontroller.h"
#include "docboy/hdma/hdma.h"
#include "docboy/ir/infrared.h"
#include "docboy/speedswitch/speedswitch.h"
#include "docboy/undoc/undocregs.h"
#endif

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/bootrom.h"
#endif

#ifdef ENABLE_BOOTROM
#ifdef ENABLE_CGB
CpuBus::CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
               Boot& boot, Apu& apu, Ppu& ppu, Dma& dma, VramBankController& vram_bank_controller,
               WramBankController& wram_bank_controller, Hdma& hdma, SpeedSwitch& speed_switch, Infrared& infrared,
               UndocumentedRegisters& undocumented_registers) :
#else
CpuBus::CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
               Boot& boot, Apu& apu, Ppu& ppu, Dma& dma) :
#endif
#else
#ifdef ENABLE_CGB
CpuBus::CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Boot& boot, Apu& apu,
               Ppu& ppu, Dma& dma, VramBankController& vram_bank_controller, WramBankController& wram_bank_controller,
               Hdma& hdma, SpeedSwitch& speed_switch, Infrared& infrared,
               UndocumentedRegisters& undocumented_registers) :
#else
CpuBus::CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Boot& boot, Apu& apu,
               Ppu& ppu, Dma& dma) :
#endif
#endif

    Bus {},

#ifdef ENABLE_BOOTROM
    boot_rom {boot_rom},
#endif
    hram {hram},
    joypad {joypad},
    serial {serial},
    timers {timers},
    interrupts {interrupts},
    boot {boot},
    apu {apu},
    ppu {ppu},
    dma {dma}
#ifdef ENABLE_CGB
    ,
    vram_bank_controller {vram_bank_controller},
    wram_bank_controller {wram_bank_controller},
    hdma {hdma},
    speed_switch {speed_switch},
    infrared {infrared},
    undocumented_registers {undocumented_registers}
#endif
{

    // clang-format off
    const NonTrivialReadFunctor read_ff {[](void*, uint16_t) -> uint8_t { return 0xFF;}, nullptr};
    const NonTrivialWriteFunctor write_nop {[](void*, uint16_t, uint8_t) {}, nullptr};
    const MemoryAccess open_bus_access {read_ff, write_nop};
    // clang-format on

#ifdef ENABLE_BOOTROM
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM::START; i <= Specs::MemoryLayout::BOOTROM::END; i++) {
        memory_accessors[i] = &boot_rom[i];
    }
#endif

    /* FF00 */ memory_accessors[Specs::Registers::Joypad::P1] = {NonTrivial<&Joypad::read_p1> {&joypad},
                                                                 NonTrivial<&Joypad::write_p1> {&joypad}};
    /* FF01 */ memory_accessors[Specs::Registers::Serial::SB] = &serial.sb;
    /* FF02 */ memory_accessors[Specs::Registers::Serial::SC] = {NonTrivial<&Serial::read_sc> {&serial},
                                                                 NonTrivial<&Serial::write_sc> {&serial}};
    /* FF03 */ memory_accessors[0xFF03] = open_bus_access;
    /* FF04 */ memory_accessors[Specs::Registers::Timers::DIV] = {NonTrivial<&Timers::read_div> {&timers},
                                                                  NonTrivial<&Timers::write_div> {&timers}};
    /* FF05 */ memory_accessors[Specs::Registers::Timers::TIMA] = {&timers.tima,
                                                                   NonTrivial<&Timers::write_tima> {&timers}};
    /* FF06 */ memory_accessors[Specs::Registers::Timers::TMA] = {&timers.tma,
                                                                  NonTrivial<&Timers::write_tma> {&timers}};
    /* FF07 */ memory_accessors[Specs::Registers::Timers::TAC] = {NonTrivial<&Timers::read_tac> {&timers},
                                                                  NonTrivial<&Timers::write_tac> {&timers}};
    /* FF08 */ memory_accessors[0xFF08] = open_bus_access;
    /* FF09 */ memory_accessors[0xFF09] = open_bus_access;
    /* FF0A */ memory_accessors[0xFF0A] = open_bus_access;
    /* FF0B */ memory_accessors[0xFF0B] = open_bus_access;
    /* FF0C */ memory_accessors[0xFF0C] = open_bus_access;
    /* FF0D */ memory_accessors[0xFF0D] = open_bus_access;
    /* FF0E */ memory_accessors[0xFF0E] = open_bus_access;
    /* FF0F */ memory_accessors[Specs::Registers::Interrupts::IF] = {&interrupts.IF,
                                                                     NonTrivial<&Interrupts::write_if> {&interrupts}};
    /* FF10 */ memory_accessors[Specs::Registers::Sound::NR10] = {NonTrivial<&Apu::read_nr10> {&apu},
                                                                  NonTrivial<&Apu::write_nr10> {&apu}};
    /* FF11 */ memory_accessors[Specs::Registers::Sound::NR11] = {NonTrivial<&Apu::read_nr11> {&apu},
                                                                  NonTrivial<&Apu::write_nr11> {&apu}};
    /* FF12 */ memory_accessors[Specs::Registers::Sound::NR12] = {NonTrivial<&Apu::read_nr12> {&apu},
                                                                  NonTrivial<&Apu::write_nr12> {&apu}};
    /* FF13 */ memory_accessors[Specs::Registers::Sound::NR13] = {NonTrivial<&Apu::read_nr13> {&apu},
                                                                  NonTrivial<&Apu::write_nr13> {&apu}};
    /* FF14 */ memory_accessors[Specs::Registers::Sound::NR14] = {NonTrivial<&Apu::read_nr14> {&apu},
                                                                  NonTrivial<&Apu::write_nr14> {&apu}};
    /* FF15 */ memory_accessors[0xFF15] = open_bus_access;
    /* FF16 */ memory_accessors[Specs::Registers::Sound::NR21] = {NonTrivial<&Apu::read_nr21> {&apu},
                                                                  NonTrivial<&Apu::write_nr21> {&apu}};
    /* FF17 */ memory_accessors[Specs::Registers::Sound::NR22] = {NonTrivial<&Apu::read_nr22> {&apu},
                                                                  NonTrivial<&Apu::write_nr22> {&apu}};
    /* FF18 */ memory_accessors[Specs::Registers::Sound::NR23] = {NonTrivial<&Apu::read_nr23> {&apu},
                                                                  NonTrivial<&Apu::write_nr23> {&apu}};
    /* FF19 */ memory_accessors[Specs::Registers::Sound::NR24] = {NonTrivial<&Apu::read_nr24> {&apu},
                                                                  NonTrivial<&Apu::write_nr24> {&apu}};
    /* FF1A */ memory_accessors[Specs::Registers::Sound::NR30] = {NonTrivial<&Apu::read_nr30> {&apu},
                                                                  NonTrivial<&Apu::write_nr30> {&apu}};
    /* FF1B */ memory_accessors[Specs::Registers::Sound::NR31] = {NonTrivial<&Apu::read_nr31> {&apu},
                                                                  NonTrivial<&Apu::write_nr31> {&apu}};
    /* FF1C */ memory_accessors[Specs::Registers::Sound::NR32] = {NonTrivial<&Apu::read_nr32> {&apu},
                                                                  NonTrivial<&Apu::write_nr32> {&apu}};
    /* FF1D */ memory_accessors[Specs::Registers::Sound::NR33] = {NonTrivial<&Apu::read_nr33> {&apu},
                                                                  NonTrivial<&Apu::write_nr33> {&apu}};
    /* FF1E */ memory_accessors[Specs::Registers::Sound::NR34] = {NonTrivial<&Apu::read_nr34> {&apu},
                                                                  NonTrivial<&Apu::write_nr34> {&apu}};
    /* FF1F */ memory_accessors[0xFF1F] = open_bus_access;
    /* FF20 */ memory_accessors[Specs::Registers::Sound::NR41] = {NonTrivial<&Apu::read_nr41> {&apu},
                                                                  NonTrivial<&Apu::write_nr41> {&apu}};
    /* FF21 */ memory_accessors[Specs::Registers::Sound::NR42] = {NonTrivial<&Apu::read_nr42> {&apu},
                                                                  NonTrivial<&Apu::write_nr42> {&apu}};
    /* FF22 */ memory_accessors[Specs::Registers::Sound::NR43] = {NonTrivial<&Apu::read_nr43> {&apu},
                                                                  NonTrivial<&Apu::write_nr43> {&apu}};
    /* FF23 */ memory_accessors[Specs::Registers::Sound::NR44] = {NonTrivial<&Apu::read_nr44> {&apu},
                                                                  NonTrivial<&Apu::write_nr44> {&apu}};
    /* FF24 */ memory_accessors[Specs::Registers::Sound::NR50] = {NonTrivial<&Apu::read_nr50> {&apu},
                                                                  NonTrivial<&Apu::write_nr50> {&apu}};
    /* FF25 */ memory_accessors[Specs::Registers::Sound::NR51] = {NonTrivial<&Apu::read_nr51> {&apu},
                                                                  NonTrivial<&Apu::write_nr51> {&apu}};
    /* FF26 */ memory_accessors[Specs::Registers::Sound::NR52] = {NonTrivial<&Apu::read_nr52> {&apu},
                                                                  NonTrivial<&Apu::write_nr52> {&apu}};
    /* FF27 */ memory_accessors[0xFF27] = open_bus_access;
    /* FF28 */ memory_accessors[0xFF28] = open_bus_access;
    /* FF29 */ memory_accessors[0xFF29] = open_bus_access;
    /* FF2A */ memory_accessors[0xFF2A] = open_bus_access;
    /* FF2B */ memory_accessors[0xFF2B] = open_bus_access;
    /* FF2C */ memory_accessors[0xFF2C] = open_bus_access;
    /* FF2D */ memory_accessors[0xFF2D] = open_bus_access;
    /* FF2E */ memory_accessors[0xFF2E] = open_bus_access;
    /* FF2F */ memory_accessors[0xFF2F] = open_bus_access;

    /* FF30 - FF3F */
    for (uint16_t i = Specs::Registers::Sound::WAVE0; i <= Specs::Registers::Sound::WAVEF; i++) {
        memory_accessors[i] = {NonTrivialRead<&Apu::read_wave_ram> {&apu},
                               NonTrivialWrite<&Apu::write_wave_ram> {&apu}};
    }

    /* FF40 */ memory_accessors[Specs::Registers::Video::LCDC] = {NonTrivial<&Ppu::read_lcdc> {&ppu},
                                                                  NonTrivial<&Ppu::write_lcdc> {&ppu}};
    /* FF41 */ memory_accessors[Specs::Registers::Video::STAT] = {NonTrivial<&Ppu::read_stat> {&ppu},
                                                                  NonTrivial<&Ppu::write_stat> {&ppu}};
    /* FF42 */ memory_accessors[Specs::Registers::Video::SCY] = &ppu.scy;
    /* FF43 */ memory_accessors[Specs::Registers::Video::SCX] = &ppu.scx;
    /* FF44 */ memory_accessors[Specs::Registers::Video::LY] = {&ppu.ly, write_nop};
    /* FF45 */ memory_accessors[Specs::Registers::Video::LYC] = &ppu.lyc;
    /* FF46 */ memory_accessors[Specs::Registers::Video::DMA] = {&ppu.dma, NonTrivial<&Ppu::write_dma> {&ppu}};
    /* FF47 */ memory_accessors[Specs::Registers::Video::BGP] = &ppu.bgp;
    /* FF48 */ memory_accessors[Specs::Registers::Video::OBP0] = &ppu.obp0;
    /* FF49 */ memory_accessors[Specs::Registers::Video::OBP1] = &ppu.obp1;
    /* FF4A */ memory_accessors[Specs::Registers::Video::WY] = &ppu.wy;
    /* FF4B */ memory_accessors[Specs::Registers::Video::WX] = &ppu.wx;
    /* FF4C */ memory_accessors[0xFF4C] = open_bus_access;
#ifdef ENABLE_CGB
    /* FF4D */ memory_accessors[Specs::Registers::SpeedSwitch::KEY1] = {
        NonTrivial<&SpeedSwitch::read_key1> {&speed_switch}, NonTrivial<&SpeedSwitch::write_key1> {&speed_switch}};
#else
    /* FF4D */ memory_accessors[0xFF4D] = open_bus_access;
#endif
    /* FF4E */ memory_accessors[0xFF4E] = open_bus_access;
#ifdef ENABLE_CGB
    /* FF4F */ memory_accessors[Specs::Registers::Banks::VBK] = {
        NonTrivial<&VramBankController::read_vbk> {&vram_bank_controller},
        NonTrivial<&VramBankController::write_vbk> {&vram_bank_controller}};
#else
    /* FF4F */ memory_accessors[0xFF4F] = open_bus_access;
#endif
    /* FF50 */ memory_accessors[Specs::Registers::Boot::BOOT] = {&boot.boot, NonTrivial<&Boot::write_boot> {&boot}};
#ifdef ENABLE_CGB
    /* FF51 */ memory_accessors[0xFF51] = {read_ff, NonTrivial<&Hdma::write_hdma1> {&hdma}};
    /* FF52 */ memory_accessors[0xFF52] = {read_ff, NonTrivial<&Hdma::write_hdma2> {&hdma}};
    /* FF53 */ memory_accessors[0xFF53] = {read_ff, NonTrivial<&Hdma::write_hdma3> {&hdma}};
    /* FF54 */ memory_accessors[0xFF54] = {read_ff, NonTrivial<&Hdma::write_hdma4> {&hdma}};
    /* FF55 */ memory_accessors[0xFF55] = {NonTrivial<&Hdma::read_hdma5> {&hdma},
                                           NonTrivial<&Hdma::write_hdma5> {&hdma}};
#else
    /* FF51 */ memory_accessors[0xFF51] = open_bus_access;
    /* FF52 */ memory_accessors[0xFF52] = open_bus_access;
    /* FF53 */ memory_accessors[0xFF53] = open_bus_access;
    /* FF54 */ memory_accessors[0xFF54] = open_bus_access;
    /* FF55 */ memory_accessors[0xFF55] = open_bus_access;
#endif
#ifdef ENABLE_CGB
    /* FF56 */ memory_accessors[Specs::Registers::Infrared::RP] = {NonTrivial<&Infrared::read_rp> {&infrared},
                                                                   NonTrivial<&Infrared::write_rp> {&infrared}};
#else
    /* FF56 */ memory_accessors[0xFF56] = open_bus_access;
#endif
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
#ifdef ENABLE_CGB
    /* FF68 */ memory_accessors[Specs::Registers::Video::BCPS] = {NonTrivial<&Ppu::read_bcps> {&ppu},
                                                                  NonTrivial<&Ppu::write_bcps> {&ppu}};
    /* FF69 */ memory_accessors[Specs::Registers::Video::BCPD] = {NonTrivial<&Ppu::read_bcpd> {&ppu},
                                                                  NonTrivial<&Ppu::write_bcpd> {&ppu}};
    /* FF6A */ memory_accessors[Specs::Registers::Video::OCPS] = {NonTrivial<&Ppu::read_ocps> {&ppu},
                                                                  NonTrivial<&Ppu::write_ocps> {&ppu}};
    /* FF6B */ memory_accessors[Specs::Registers::Video::OCPD] = {NonTrivial<&Ppu::read_ocpd> {&ppu},
                                                                  NonTrivial<&Ppu::write_ocpd> {&ppu}};
    /* FF6C */ memory_accessors[Specs::Registers::Video::OPRI] = {NonTrivial<&Ppu::read_opri> {&ppu},
                                                                  NonTrivial<&Ppu::write_opri> {&ppu}};
#else
    /* FF68 */ memory_accessors[0xFF68] = open_bus_access;
    /* FF69 */ memory_accessors[0xFF69] = open_bus_access;
    /* FF6A */ memory_accessors[0xFF6A] = open_bus_access;
    /* FF6B */ memory_accessors[0xFF6B] = open_bus_access;
    /* FF6C */ memory_accessors[0xFF6C] = open_bus_access;
#endif
    /* FF6D */ memory_accessors[0xFF6D] = open_bus_access;
    /* FF6E */ memory_accessors[0xFF6E] = open_bus_access;
    /* FF6F */ memory_accessors[0xFF6F] = open_bus_access;
#ifdef ENABLE_CGB
    /* FF70 */ memory_accessors[Specs::Registers::Banks::SVBK] = {
        NonTrivial<&WramBankController::read_svbk> {&wram_bank_controller},
        NonTrivial<&WramBankController::write_svbk> {&wram_bank_controller}};
#else
    /* FF70 */ memory_accessors[0xFF70] = open_bus_access;
#endif

    /* FF71 */ memory_accessors[0xFF71] = open_bus_access;

#ifdef ENABLE_CGB
    /* FF72 */ memory_accessors[Specs::Registers::Undocumented::FF72] = &undocumented_registers.ff72;
    /* FF73 */ memory_accessors[Specs::Registers::Undocumented::FF73] = &undocumented_registers.ff73;
    /* FF74 */ memory_accessors[Specs::Registers::Undocumented::FF74] = &undocumented_registers.ff74;
    /* FF75 */ memory_accessors[Specs::Registers::Undocumented::FF75] = {
        &undocumented_registers.ff75, NonTrivial<&UndocumentedRegisters::write_ff75> {&undocumented_registers}};
#else
    /* FF72 */ memory_accessors[0xFF72] = open_bus_access;
    /* FF73 */ memory_accessors[0xFF73] = open_bus_access;
    /* FF74 */ memory_accessors[0xFF74] = open_bus_access;
    /* FF75 */ memory_accessors[0xFF75] = open_bus_access;
#endif

#ifdef ENABLE_CGB
    /* FF76 */ memory_accessors[Specs::Registers::Sound::PCM12] = {NonTrivial<&Apu::read_pcm12> {&apu}, write_nop};
    /* FF76 */ memory_accessors[Specs::Registers::Sound::PCM34] = {NonTrivial<&Apu::read_pcm34> {&apu}, write_nop};
#else
    /* FF76 */ memory_accessors[0xFF76] = open_bus_access;
    /* FF77 */ memory_accessors[0xFF77] = open_bus_access;
#endif
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
