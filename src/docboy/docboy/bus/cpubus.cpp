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
#include "docboy/mode/operatingmode.h"
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
               WramBankController& wram_bank_controller, Hdma& hdma, OperatingMode& operating_mode,
               SpeedSwitch& speed_switch, Infrared& infrared, UndocumentedRegisters& undocumented_registers) :
#else
CpuBus::CpuBus(BootRom& boot_rom, Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts,
               Boot& boot, Apu& apu, Ppu& ppu, Dma& dma) :
#endif
#else
#ifdef ENABLE_CGB
CpuBus::CpuBus(Hram& hram, Joypad& joypad, Serial& serial, Timers& timers, Interrupts& interrupts, Boot& boot, Apu& apu,
               Ppu& ppu, Dma& dma, VramBankController& vram_bank_controller, WramBankController& wram_bank_controller,
               Hdma& hdma, OperatingMode& operating_mode, SpeedSwitch& speed_switch, Infrared& infrared,
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
    operating_mode {operating_mode},
    speed_switch {speed_switch},
    infrared {infrared},
    undocumented_registers {undocumented_registers}
#endif
{
#ifdef ENABLE_BOOTROM
    /* 0x0000 - 0x00FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM0::START; i <= Specs::MemoryLayout::BOOTROM0::END; i++) {
        memory_accessors[i] = &boot_rom[i];
    }
#ifdef ENABLE_CGB
    // On CGB bios is split in two (with cartridge header in between)
    /* 0x0200 - 0x08FF */
    for (uint16_t i = Specs::MemoryLayout::BOOTROM1::START; i <= Specs::MemoryLayout::BOOTROM1::END; i++) {
        memory_accessors[i] = &boot_rom[i];
    }
#endif
#endif

    /* FF00 */ memory_accessors[Specs::Registers::Joypad::P1] = {NonTrivial<&Joypad::read_p1> {&joypad},
                                                                 NonTrivial<&Joypad::write_p1> {&joypad}};
    /* FF01 */ memory_accessors[Specs::Registers::Serial::SB] = &serial.sb;
#ifdef ENABLE_CGB
    /* FF02 */ memory_accessors[Specs::Registers::Serial::SC] = {NonTrivial<&Serial::read_sc_cgb> {&serial},
                                                                 NonTrivial<&Serial::write_sc_cgb> {&serial}};
#else
    /* FF02 */ memory_accessors[Specs::Registers::Serial::SC] = {NonTrivial<&Serial::read_sc_dmg> {&serial},
                                                                 NonTrivial<&Serial::write_sc_dmg> {&serial}};
#endif
    /* FF03 */ memory_accessors[0xFF03] = OPEN_BUS;
    /* FF04 */ memory_accessors[Specs::Registers::Timers::DIV] = {NonTrivial<&Timers::read_div> {&timers},
                                                                  NonTrivial<&Timers::write_div> {&timers}};
    /* FF05 */ memory_accessors[Specs::Registers::Timers::TIMA] = {&timers.tima,
                                                                   NonTrivial<&Timers::write_tima> {&timers}};
    /* FF06 */ memory_accessors[Specs::Registers::Timers::TMA] = {&timers.tma,
                                                                  NonTrivial<&Timers::write_tma> {&timers}};
    /* FF07 */ memory_accessors[Specs::Registers::Timers::TAC] = {NonTrivial<&Timers::read_tac> {&timers},
                                                                  NonTrivial<&Timers::write_tac> {&timers}};
    /* FF08 */ memory_accessors[0xFF08] = OPEN_BUS;
    /* FF09 */ memory_accessors[0xFF09] = OPEN_BUS;
    /* FF0A */ memory_accessors[0xFF0A] = OPEN_BUS;
    /* FF0B */ memory_accessors[0xFF0B] = OPEN_BUS;
    /* FF0C */ memory_accessors[0xFF0C] = OPEN_BUS;
    /* FF0D */ memory_accessors[0xFF0D] = OPEN_BUS;
    /* FF0E */ memory_accessors[0xFF0E] = OPEN_BUS;
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
    /* FF15 */ memory_accessors[0xFF15] = OPEN_BUS;
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
    /* FF1F */ memory_accessors[0xFF1F] = OPEN_BUS;
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
    /* FF27 */ memory_accessors[0xFF27] = OPEN_BUS;
    /* FF28 */ memory_accessors[0xFF28] = OPEN_BUS;
    /* FF29 */ memory_accessors[0xFF29] = OPEN_BUS;
    /* FF2A */ memory_accessors[0xFF2A] = OPEN_BUS;
    /* FF2B */ memory_accessors[0xFF2B] = OPEN_BUS;
    /* FF2C */ memory_accessors[0xFF2C] = OPEN_BUS;
    /* FF2D */ memory_accessors[0xFF2D] = OPEN_BUS;
    /* FF2E */ memory_accessors[0xFF2E] = OPEN_BUS;
    /* FF2F */ memory_accessors[0xFF2F] = OPEN_BUS;

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
    /* FF44 */ memory_accessors[Specs::Registers::Video::LY] = {&ppu.ly, WRITE_NOP};
    /* FF45 */ memory_accessors[Specs::Registers::Video::LYC] = &ppu.lyc;
    /* FF46 */ memory_accessors[Specs::Registers::Video::DMA] = {&ppu.dma, NonTrivial<&Ppu::write_dma> {&ppu}};
    /* FF47 */ memory_accessors[Specs::Registers::Video::BGP] = &ppu.bgp;
    /* FF48 */ memory_accessors[Specs::Registers::Video::OBP0] = &ppu.obp0;
    /* FF49 */ memory_accessors[Specs::Registers::Video::OBP1] = &ppu.obp1;
    /* FF4A */ memory_accessors[Specs::Registers::Video::WY] = &ppu.wy;
    /* FF4B */ memory_accessors[Specs::Registers::Video::WX] = &ppu.wx;
#ifdef ENABLE_CGB
#ifdef ENABLE_BOOTROM
    /* FF4C */ memory_accessors[Specs::Registers::OperatingMode::KEY0] = {
        NonTrivial<&OperatingMode::read_key0> {&operating_mode},
        NonTrivial<&OperatingMode::write_key0> {&operating_mode}};
#else
    /* FF4C */ memory_accessors[Specs::Registers::OperatingMode::KEY0] = OPEN_BUS;
#endif
#else
    /* FF4C */ memory_accessors[0xFF4C] = OPEN_BUS;
#endif
#ifdef ENABLE_CGB
    /* FF4D */ memory_accessors[Specs::Registers::SpeedSwitch::KEY1] = {
        NonTrivial<&SpeedSwitch::read_key1> {&speed_switch}, NonTrivial<&SpeedSwitch::write_key1> {&speed_switch}};
#else
    /* FF4D */ memory_accessors[0xFF4D] = OPEN_BUS;
#endif
    /* FF4E */ memory_accessors[0xFF4E] = OPEN_BUS;
#ifdef ENABLE_CGB
    /* FF4F */ memory_accessors[Specs::Registers::Banks::VBK] = {
        NonTrivial<&VramBankController::read_vbk> {&vram_bank_controller},
        NonTrivial<&VramBankController::write_vbk> {&vram_bank_controller}};
#else
    /* FF4F */ memory_accessors[0xFF4F] = OPEN_BUS;
#endif
    /* FF50 */ memory_accessors[Specs::Registers::Boot::BOOT] = {&boot.boot, NonTrivial<&Boot::write_boot> {&boot}};
#ifdef ENABLE_CGB
    /* FF51 */ memory_accessors[Specs::Registers::Hdma::HDMA1] = {READ_FF, NonTrivial<&Hdma::write_hdma1> {&hdma}};
    /* FF52 */ memory_accessors[Specs::Registers::Hdma::HDMA2] = {READ_FF, NonTrivial<&Hdma::write_hdma2> {&hdma}};
    /* FF53 */ memory_accessors[Specs::Registers::Hdma::HDMA3] = {READ_FF, NonTrivial<&Hdma::write_hdma3> {&hdma}};
    /* FF54 */ memory_accessors[Specs::Registers::Hdma::HDMA4] = {READ_FF, NonTrivial<&Hdma::write_hdma4> {&hdma}};
    /* FF55 */ memory_accessors[Specs::Registers::Hdma::HDMA5] = {NonTrivial<&Hdma::read_hdma5> {&hdma},
                                                                  NonTrivial<&Hdma::write_hdma5> {&hdma}};
#else
    /* FF51 */ memory_accessors[0xFF51] = OPEN_BUS;
    /* FF52 */ memory_accessors[0xFF52] = OPEN_BUS;
    /* FF53 */ memory_accessors[0xFF53] = OPEN_BUS;
    /* FF54 */ memory_accessors[0xFF54] = OPEN_BUS;
    /* FF55 */ memory_accessors[0xFF55] = OPEN_BUS;
#endif
#ifdef ENABLE_CGB
    /* FF56 */ memory_accessors[Specs::Registers::Infrared::RP] = {NonTrivial<&Infrared::read_rp> {&infrared},
                                                                   NonTrivial<&Infrared::write_rp> {&infrared}};
#else
    /* FF56 */ memory_accessors[0xFF56] = OPEN_BUS;
#endif
    /* FF57 */ memory_accessors[0xFF57] = OPEN_BUS;
    /* FF58 */ memory_accessors[0xFF58] = OPEN_BUS;
    /* FF59 */ memory_accessors[0xFF59] = OPEN_BUS;
    /* FF5A */ memory_accessors[0xFF5A] = OPEN_BUS;
    /* FF5B */ memory_accessors[0xFF5B] = OPEN_BUS;
    /* FF5C */ memory_accessors[0xFF5C] = OPEN_BUS;
    /* FF5D */ memory_accessors[0xFF5D] = OPEN_BUS;
    /* FF5E */ memory_accessors[0xFF5E] = OPEN_BUS;
    /* FF5F */ memory_accessors[0xFF5F] = OPEN_BUS;
    /* FF60 */ memory_accessors[0xFF60] = OPEN_BUS;
    /* FF61 */ memory_accessors[0xFF61] = OPEN_BUS;
    /* FF62 */ memory_accessors[0xFF62] = OPEN_BUS;
    /* FF63 */ memory_accessors[0xFF63] = OPEN_BUS;
    /* FF64 */ memory_accessors[0xFF64] = OPEN_BUS;
    /* FF65 */ memory_accessors[0xFF65] = OPEN_BUS;
    /* FF66 */ memory_accessors[0xFF66] = OPEN_BUS;
    /* FF67 */ memory_accessors[0xFF67] = OPEN_BUS;
#ifdef ENABLE_CGB
    /* FF68 */ memory_accessors[Specs::Registers::Video::BCPS] = {NonTrivial<&Ppu::read_bcps> {&ppu},
                                                                  NonTrivial<&Ppu::write_bcps> {&ppu}};
    /* FF69 */ memory_accessors[Specs::Registers::Video::BCPD] = {NonTrivial<&Ppu::read_bcpd> {&ppu},
                                                                  NonTrivial<&Ppu::write_bcpd> {&ppu}};
    /* FF6A */ memory_accessors[Specs::Registers::Video::OCPS] = {NonTrivial<&Ppu::read_ocps> {&ppu},
                                                                  NonTrivial<&Ppu::write_ocps> {&ppu}};
    /* FF6B */ memory_accessors[Specs::Registers::Video::OCPD] = {NonTrivial<&Ppu::read_ocpd> {&ppu},
                                                                  NonTrivial<&Ppu::write_ocpd> {&ppu}};
#ifdef ENABLE_BOOTROM
    /* FF6C */ memory_accessors[Specs::Registers::Video::OPRI] = {NonTrivial<&Ppu::read_opri> {&ppu},
                                                                  NonTrivial<&Ppu::write_opri_effective> {&ppu}};
#else
    /* FF6C */ memory_accessors[Specs::Registers::Video::OPRI] = {NonTrivial<&Ppu::read_opri> {&ppu},
                                                                  NonTrivial<&Ppu::write_opri> {&ppu}};
#endif
#else
    /* FF68 */ memory_accessors[0xFF68] = OPEN_BUS;
    /* FF69 */ memory_accessors[0xFF69] = OPEN_BUS;
    /* FF6A */ memory_accessors[0xFF6A] = OPEN_BUS;
    /* FF6B */ memory_accessors[0xFF6B] = OPEN_BUS;
    /* FF6C */ memory_accessors[0xFF6C] = OPEN_BUS;
#endif
    /* FF6D */ memory_accessors[0xFF6D] = OPEN_BUS;
    /* FF6E */ memory_accessors[0xFF6E] = OPEN_BUS;
    /* FF6F */ memory_accessors[0xFF6F] = OPEN_BUS;
#ifdef ENABLE_CGB
    /* FF70 */ memory_accessors[Specs::Registers::Banks::SVBK] = {
        NonTrivial<&WramBankController::read_svbk> {&wram_bank_controller},
        NonTrivial<&WramBankController::write_svbk> {&wram_bank_controller}};
#else
    /* FF70 */ memory_accessors[0xFF70] = OPEN_BUS;
#endif

    /* FF71 */ memory_accessors[0xFF71] = OPEN_BUS;

#ifdef ENABLE_CGB
    /* FF72 */ memory_accessors[Specs::Registers::Undocumented::FF72] = &undocumented_registers.ff72;
    /* FF73 */ memory_accessors[Specs::Registers::Undocumented::FF73] = &undocumented_registers.ff73;
    /* FF74 */ memory_accessors[Specs::Registers::Undocumented::FF74] = &undocumented_registers.ff74;
    /* FF75 */ memory_accessors[Specs::Registers::Undocumented::FF75] = {
        &undocumented_registers.ff75, NonTrivial<&UndocumentedRegisters::write_ff75> {&undocumented_registers}};
#else
    /* FF72 */ memory_accessors[0xFF72] = OPEN_BUS;
    /* FF73 */ memory_accessors[0xFF73] = OPEN_BUS;
    /* FF74 */ memory_accessors[0xFF74] = OPEN_BUS;
    /* FF75 */ memory_accessors[0xFF75] = OPEN_BUS;
#endif

#ifdef ENABLE_CGB
    /* FF76 */ memory_accessors[Specs::Registers::Sound::PCM12] = {NonTrivial<&Apu::read_pcm12> {&apu}, WRITE_NOP};
    /* FF76 */ memory_accessors[Specs::Registers::Sound::PCM34] = {NonTrivial<&Apu::read_pcm34> {&apu}, WRITE_NOP};
#else
    /* FF76 */ memory_accessors[0xFF76] = OPEN_BUS;
    /* FF77 */ memory_accessors[0xFF77] = OPEN_BUS;
#endif
    /* FF78 */ memory_accessors[0xFF78] = OPEN_BUS;
    /* FF79 */ memory_accessors[0xFF79] = OPEN_BUS;
    /* FF7A */ memory_accessors[0xFF7A] = OPEN_BUS;
    /* FF7B */ memory_accessors[0xFF7B] = OPEN_BUS;
    /* FF7C */ memory_accessors[0xFF7C] = OPEN_BUS;
    /* FF7D */ memory_accessors[0xFF7D] = OPEN_BUS;
    /* FF7E */ memory_accessors[0xFF7E] = OPEN_BUS;
    /* FF7F */ memory_accessors[0xFF7F] = OPEN_BUS;

    /* 0xFF80 - 0xFFFE */
    for (uint16_t i = Specs::MemoryLayout::HRAM::START; i <= Specs::MemoryLayout::HRAM::END; i++) {
        memory_accessors[i] = &hram[i - Specs::MemoryLayout::HRAM::START];
    }

    /* FFFF */ memory_accessors[Specs::MemoryLayout::IE] = &interrupts.IE;
}

void CpuBus::save_state(Parcel& parcel) const {
    Bus::save_state(parcel);
#if defined(ENABLE_CGB) && defined(ENABLE_BOOTROM)
    PARCEL_WRITE_BOOL(parcel, boot_rom_locked);
#endif
}

void CpuBus::load_state(Parcel& parcel) {
    Bus::load_state(parcel);
#ifdef ENABLE_CGB
#ifdef ENABLE_BOOTROM
    boot_rom_locked = parcel.read_bool();
#endif
    init_accessors_for_operating_mode();
#endif
}

void CpuBus::reset() {
    Bus::reset();
    address = if_bootrom_else(0, 0xFF50);
    data = if_bootrom_else(0xFF, 0x01);
    decay = if_bootrom_else(0, 3);
#ifdef ENABLE_CGB
#ifdef ENABLE_BOOTROM
    boot_rom_locked = false;
#endif
    init_accessors_for_operating_mode();
#endif
}

#if defined(ENABLE_CGB) && defined(ENABLE_BOOTROM)
void CpuBus::lock_boot_rom() {
    ASSERT(!boot_rom_locked);
    boot_rom_locked = true;
    init_accessors_for_operating_mode();
}
#endif

#ifdef ENABLE_CGB
void CpuBus::init_accessors_for_operating_mode() {
#ifdef ENABLE_BOOTROM
    if (boot_rom_locked) {
        // After boot rom is unmapped, KEY0 (CGB/DMG mode) is unmapped as well and can't be written anymore.
        /* FF4C */ memory_accessors[Specs::Registers::OperatingMode::KEY0] = OPEN_BUS;
    } else {
        /* FF4C */ memory_accessors[Specs::Registers::OperatingMode::KEY0] = {
            NonTrivial<&OperatingMode::read_key0> {&operating_mode},
            NonTrivial<&OperatingMode::write_key0> {&operating_mode}};
    }
#endif

    // Enable or disable registers based on the current operating mode.
    //
    // +----------+----------+--------------+----------+
    // | Function | CGB mode | DMG ext mode | DMG mode |
    // +----------+----------+--------------+----------+
    // | BGPS     |   yes    |      yes     |    yes   |
    // | BGPD     |   yes    |      no      |    no    |
    // | OBPS     |   yes    |      yes     |    yes   |
    // | OBPD     |   yes    |      no      |    no    |
    // | OPRI     |   yes    |      yes     |    no    |
    // | VBK      |   yes    |      yes     |    no    |
    // | SVBK     |   yes    |      yes     |    no    |
    // | HDMA1    |   yes    |      no      |    no    |
    // | HDMA2    |   yes    |      no      |    no    |
    // | HDMA3    |   yes    |      no      |    no    |
    // | HDMA4    |   yes    |      no      |    no    |
    // | HDMA5    |   yes    |      no      |    no    |
    // | KEY1     |   yes    |      yes     |    no    |
    // | RP       |   yes    |      yes     |    no    |
    // | SC[1]    |   yes    |      yes     |    no    |
    // | PCM12    |   yes    |      yes     |    yes   |
    // | PCM34    |   yes    |      yes     |    yes   |
    // | FF72     |   yes    |      yes     |    yes   |
    // | FF73     |   yes    |      yes     |    yes   |
    // | FF74     |   yes    |      yes     |    yes   |
    // | FF75     |   yes    |      yes     |    yes   |
    // +----------+----------+--------------+----------+

    if (operating_mode.key0.dmg_mode || operating_mode.key0.dmg_ext_mode) {
        /* FF51 */ memory_accessors[Specs::Registers::Hdma::HDMA1] = OPEN_BUS;
        /* FF52 */ memory_accessors[Specs::Registers::Hdma::HDMA2] = OPEN_BUS;
        /* FF53 */ memory_accessors[Specs::Registers::Hdma::HDMA3] = OPEN_BUS;
        /* FF54 */ memory_accessors[Specs::Registers::Hdma::HDMA4] = OPEN_BUS;
        /* FF55 */ memory_accessors[Specs::Registers::Hdma::HDMA5] = OPEN_BUS;

        /* FF69 */ memory_accessors[Specs::Registers::Video::BCPD] = OPEN_BUS;
        /* FF6B */ memory_accessors[Specs::Registers::Video::OCPD] = OPEN_BUS;

        if (operating_mode.key0.dmg_ext_mode) {
            // DMG ext mode.

            /* FF02 */ memory_accessors[Specs::Registers::Serial::SC] = {NonTrivial<&Serial::read_sc_cgb> {&serial},
                                                                         NonTrivial<&Serial::write_sc_cgb> {&serial}};

            /* FF4D */ memory_accessors[Specs::Registers::SpeedSwitch::KEY1] = {
                NonTrivial<&SpeedSwitch::read_key1> {&speed_switch},
                NonTrivial<&SpeedSwitch::write_key1> {&speed_switch}};

            /* FF56 */ memory_accessors[Specs::Registers::Infrared::RP] = {NonTrivial<&Infrared::read_rp> {&infrared},
                                                                           NonTrivial<&Infrared::write_rp> {&infrared}};
#ifdef ENABLE_BOOTROM
            /* FF6C */ memory_accessors[Specs::Registers::Video::OPRI] = {
                NonTrivial<&Ppu::read_opri> {&ppu}, NonTrivial<&Ppu::write_opri_effective> {&ppu}};
#else
            /* FF6C */ memory_accessors[Specs::Registers::Video::OPRI] = {NonTrivial<&Ppu::read_opri> {&ppu},
                                                                          NonTrivial<&Ppu::write_opri> {&ppu}};
#endif

            /* FF4F */ memory_accessors[Specs::Registers::Banks::VBK] = {
                NonTrivial<&VramBankController::read_vbk> {&vram_bank_controller},
                NonTrivial<&VramBankController::write_vbk> {&vram_bank_controller}};
            /* FF70 */ memory_accessors[Specs::Registers::Banks::SVBK] = {
                NonTrivial<&WramBankController::read_svbk> {&wram_bank_controller},
                NonTrivial<&WramBankController::write_svbk> {&wram_bank_controller}};
        } else {
            // DMG mode.

            /* FF02 */ memory_accessors[Specs::Registers::Serial::SC] = {NonTrivial<&Serial::read_sc_dmg> {&serial},
                                                                         NonTrivial<&Serial::write_sc_dmg> {&serial}};

            /* FF4D */ memory_accessors[Specs::Registers::SpeedSwitch::KEY1] = OPEN_BUS;

            /* FF56 */ memory_accessors[Specs::Registers::Infrared::RP] = OPEN_BUS;

            /* FF6C */ memory_accessors[Specs::Registers::Video::OPRI] = OPEN_BUS;

            /* FF4F */ memory_accessors[Specs::Registers::Banks::VBK] = {
                NonTrivial<&VramBankController::read_vbk> {&vram_bank_controller}, WRITE_NOP};
            /* FF70 */ memory_accessors[Specs::Registers::Banks::SVBK] = OPEN_BUS;
        }
    } else {
        /* FF02 */ memory_accessors[Specs::Registers::Serial::SC] = {NonTrivial<&Serial::read_sc_cgb> {&serial},
                                                                     NonTrivial<&Serial::write_sc_cgb> {&serial}};

        /* FF51 */ memory_accessors[Specs::Registers::Hdma::HDMA1] = {READ_FF, NonTrivial<&Hdma::write_hdma1> {&hdma}};
        /* FF52 */ memory_accessors[Specs::Registers::Hdma::HDMA2] = {READ_FF, NonTrivial<&Hdma::write_hdma2> {&hdma}};
        /* FF53 */ memory_accessors[Specs::Registers::Hdma::HDMA3] = {READ_FF, NonTrivial<&Hdma::write_hdma3> {&hdma}};
        /* FF54 */ memory_accessors[Specs::Registers::Hdma::HDMA4] = {READ_FF, NonTrivial<&Hdma::write_hdma4> {&hdma}};
        /* FF55 */ memory_accessors[Specs::Registers::Hdma::HDMA5] = {NonTrivial<&Hdma::read_hdma5> {&hdma},
                                                                      NonTrivial<&Hdma::write_hdma5> {&hdma}};

        /* FF56 */ memory_accessors[Specs::Registers::Infrared::RP] = {NonTrivial<&Infrared::read_rp> {&infrared},
                                                                       NonTrivial<&Infrared::write_rp> {&infrared}};

        /* FF69 */ memory_accessors[Specs::Registers::Video::BCPD] = {NonTrivial<&Ppu::read_bcpd> {&ppu},
                                                                      NonTrivial<&Ppu::write_bcpd> {&ppu}};
        /* FF6B */ memory_accessors[Specs::Registers::Video::OCPD] = {NonTrivial<&Ppu::read_ocpd> {&ppu},
                                                                      NonTrivial<&Ppu::write_ocpd> {&ppu}};
#ifdef ENABLE_BOOTROM
        /* FF6C */ memory_accessors[Specs::Registers::Video::OPRI] = {NonTrivial<&Ppu::read_opri> {&ppu},
                                                                      NonTrivial<&Ppu::write_opri_effective> {&ppu}};
#else
        /* FF6C */ memory_accessors[Specs::Registers::Video::OPRI] = {NonTrivial<&Ppu::read_opri> {&ppu},
                                                                      NonTrivial<&Ppu::write_opri> {&ppu}};
#endif

        /* FF4F */ memory_accessors[Specs::Registers::Banks::VBK] = {
            NonTrivial<&VramBankController::read_vbk> {&vram_bank_controller},
            NonTrivial<&VramBankController::write_vbk> {&vram_bank_controller}};
        /* FF70 */ memory_accessors[Specs::Registers::Banks::SVBK] = {
            NonTrivial<&WramBankController::read_svbk> {&wram_bank_controller},
            NonTrivial<&WramBankController::write_svbk> {&wram_bank_controller}};
    }
}
#endif