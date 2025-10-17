#ifndef SPECS_H
#define SPECS_H

#include <cstdint>

#include "utils/bitrange.h"

namespace Specs {
namespace Display {
    constexpr uint32_t WIDTH = 160;
    constexpr uint32_t HEIGHT = 144;
} // namespace Display

namespace Frequencies {
    constexpr uint32_t CLOCK = 4194304;
    constexpr uint32_t PPU = 4194304;
    constexpr uint32_t CPU = 1048576;
    constexpr uint32_t SERIAL = 8192;
    constexpr uint32_t DIV = 16384;
    constexpr uint32_t TAC[] = {4096, 262144, 65536, 16384};
} // namespace Frequencies

namespace Ppu {
    constexpr uint32_t DOTS_PER_FRAME = 70224;
    namespace Modes {
        constexpr uint8_t HBLANK = 0;
        constexpr uint8_t VBLANK = 1;
        constexpr uint8_t OAM_SCAN = 2;
        constexpr uint8_t PIXEL_TRANSFER = 3;
    } // namespace Modes
} // namespace Ppu

namespace MemoryLayout {
    namespace BOOTROM {
        constexpr uint16_t START = 0x0000;
        constexpr uint16_t END = 0x00FF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace BOOTROM
    namespace ROM0 {
        constexpr uint16_t START = 0x0000;
        constexpr uint16_t END = 0x3FFF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace ROM0
    namespace ROM1 {
        constexpr uint16_t START = 0x4000;
        constexpr uint16_t END = 0x7FFF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace ROM1
    namespace VRAM {
        constexpr uint16_t START = 0x8000;
        constexpr uint16_t END = 0x9FFF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace VRAM
    namespace RAM {
        constexpr uint16_t START = 0xA000;
        constexpr uint16_t END = 0xBFFF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace RAM
    namespace WRAM1 {
        constexpr uint16_t START = 0xC000;
        constexpr uint16_t END = 0xCFFF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace WRAM1
    namespace WRAM2 {
        constexpr uint16_t START = 0xD000;
        constexpr uint16_t END = 0xDFFF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace WRAM2
    namespace ECHO_RAM {
        constexpr uint16_t START = 0xE000;
        constexpr uint16_t END = 0xFDFF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace ECHO_RAM
    namespace OAM {
        constexpr uint16_t START = 0xFE00;
        constexpr uint16_t END = 0xFE9F;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace OAM
    namespace NOT_USABLE {
        constexpr uint16_t START = 0xFEA0;
        constexpr uint16_t END = 0xFEFF;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace NOT_USABLE

    namespace IO {
        constexpr uint16_t START = 0xFF00;
        constexpr uint16_t END = 0xFF7F;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace IO

    namespace HRAM {
        constexpr uint16_t START = 0xFF80;
        constexpr uint16_t END = 0xFFFE;
        constexpr uint16_t SIZE = END - START + 1;
    } // namespace HRAM

    constexpr uint16_t IE = 0xFFFF;
} // namespace MemoryLayout

namespace Registers {
    namespace Joypad {
        constexpr uint16_t P1 = 0xFF00;
        constexpr uint16_t REGISTERS[] = {P1};
    } // namespace Joypad

    namespace Serial {
        constexpr uint16_t SB = 0xFF01;
        constexpr uint16_t SC = 0xFF02;
        constexpr uint16_t REGISTERS[] = {SB, SC};
    } // namespace Serial

    namespace Timers {
        constexpr uint16_t DIV = 0xFF04;
        constexpr uint16_t TIMA = 0xFF05;
        constexpr uint16_t TMA = 0xFF06;
        constexpr uint16_t TAC = 0xFF07;
        constexpr uint16_t REGISTERS[] = {DIV, TIMA, TMA, TAC};
    } // namespace Timers

    namespace Interrupts {
        constexpr uint16_t IF = 0xFF0F;
        constexpr uint16_t IE = 0xFFFF;
        constexpr uint16_t REGISTERS[] = {IF, IE};
    } // namespace Interrupts

    namespace Sound {
        constexpr uint16_t NR10 = 0xFF10;
        constexpr uint16_t NR11 = 0xFF11;
        constexpr uint16_t NR12 = 0xFF12;
        constexpr uint16_t NR13 = 0xFF13;
        constexpr uint16_t NR14 = 0xFF14;
        constexpr uint16_t NR21 = 0xFF16;
        constexpr uint16_t NR22 = 0xFF17;
        constexpr uint16_t NR23 = 0xFF18;
        constexpr uint16_t NR24 = 0xFF19;
        constexpr uint16_t NR30 = 0xFF1A;
        constexpr uint16_t NR31 = 0xFF1B;
        constexpr uint16_t NR32 = 0xFF1C;
        constexpr uint16_t NR33 = 0xFF1D;
        constexpr uint16_t NR34 = 0xFF1E;
        constexpr uint16_t NR41 = 0xFF20;
        constexpr uint16_t NR42 = 0xFF21;
        constexpr uint16_t NR43 = 0xFF22;
        constexpr uint16_t NR44 = 0xFF23;
        constexpr uint16_t NR50 = 0xFF24;
        constexpr uint16_t NR51 = 0xFF25;
        constexpr uint16_t NR52 = 0xFF26;
        constexpr uint16_t WAVE0 = 0xFF30;
        constexpr uint16_t WAVE1 = 0xFF31;
        constexpr uint16_t WAVE2 = 0xFF32;
        constexpr uint16_t WAVE3 = 0xFF33;
        constexpr uint16_t WAVE4 = 0xFF34;
        constexpr uint16_t WAVE5 = 0xFF35;
        constexpr uint16_t WAVE6 = 0xFF36;
        constexpr uint16_t WAVE7 = 0xFF37;
        constexpr uint16_t WAVE8 = 0xFF38;
        constexpr uint16_t WAVE9 = 0xFF39;
        constexpr uint16_t WAVEA = 0xFF3A;
        constexpr uint16_t WAVEB = 0xFF3B;
        constexpr uint16_t WAVEC = 0xFF3C;
        constexpr uint16_t WAVED = 0xFF3D;
        constexpr uint16_t WAVEE = 0xFF3E;
        constexpr uint16_t WAVEF = 0xFF3F;
#ifdef ENABLE_CGB
        constexpr uint16_t PCM12 = 0xFF76;
        constexpr uint16_t PCM34 = 0xFF77;
#endif
        constexpr uint16_t REGISTERS[] = {NR10,  NR11,  NR12,  NR13,  NR14,  NR21,  NR22,  NR23,  NR24,  NR30,
                                          NR31,  NR32,  NR33,  NR34,  NR41,  NR42,  NR43,  NR44,  NR50,  NR51,
                                          NR52,  WAVE0, WAVE1, WAVE2, WAVE3, WAVE4, WAVE5, WAVE6, WAVE7, WAVE8,
                                          WAVE9, WAVEA, WAVEB, WAVEC, WAVED, WAVEE, WAVEF};
#ifdef ENABLE_CGB
        constexpr uint16_t CGB_REGISTERS[] = {PCM12, PCM34};
#endif
    } // namespace Sound

    namespace Video {
        constexpr uint16_t LCDC = 0xFF40;
        constexpr uint16_t STAT = 0xFF41;
        constexpr uint16_t SCY = 0xFF42;
        constexpr uint16_t SCX = 0xFF43;
        constexpr uint16_t LY = 0xFF44;
        constexpr uint16_t LYC = 0xFF45;
        constexpr uint16_t DMA = 0xFF46;
        constexpr uint16_t BGP = 0xFF47;
        constexpr uint16_t OBP0 = 0xFF48;
        constexpr uint16_t OBP1 = 0xFF49;
        constexpr uint16_t WY = 0xFF4A;
        constexpr uint16_t WX = 0xFF4B;
#ifdef ENABLE_CGB
        constexpr uint16_t BCPS = 0xFF68;
        constexpr uint16_t BCPD = 0xFF69;
        constexpr uint16_t OCPS = 0xFF6A;
        constexpr uint16_t OCPD = 0xFF6B;
        constexpr uint16_t OPRI = 0xFF6C;
#endif

        constexpr uint16_t REGISTERS[] = {LCDC, STAT, SCY, SCX, LY, LYC, DMA, BGP, OBP0, OBP1, WY, WX};

#ifdef ENABLE_CGB
        constexpr uint16_t CGB_REGISTERS[] = {BCPS, BCPD, OCPS, OCPD, OPRI};
#endif
    } // namespace Video

#ifdef ENABLE_CGB
    namespace SpeedSwitch {
        constexpr uint16_t KEY1 = 0xFF4D;
        constexpr uint16_t CGB_REGISTERS[] = {KEY1};
    } // namespace SpeedSwitch

    namespace Hdma {
        constexpr uint16_t HDMA1 = 0xFF51;
        constexpr uint16_t HDMA2 = 0xFF52;
        constexpr uint16_t HDMA3 = 0xFF53;
        constexpr uint16_t HDMA4 = 0xFF54;
        constexpr uint16_t HDMA5 = 0xFF55;
        constexpr uint16_t CGB_REGISTERS[] = {HDMA1, HDMA2, HDMA3, HDMA4, HDMA5};
    } // namespace Hdma

    namespace Infrared {
        constexpr uint16_t RP = 0xFF56;
        constexpr uint16_t CGB_REGISTERS[] = {RP};
    } // namespace Infrared

    namespace Banks {
        constexpr uint16_t VBK = 0xFF4F;
        constexpr uint16_t SVBK = 0xFF70;

        constexpr uint16_t CGB_REGISTERS[] = {VBK, SVBK};
    } // namespace Banks

    namespace Undocumented {
        constexpr uint16_t FF72 = 0xFF72;
        constexpr uint16_t FF73 = 0xFF73;
        constexpr uint16_t FF74 = 0xFF74;
        constexpr uint16_t FF75 = 0xFF75;

        constexpr uint16_t CGB_REGISTERS[] = {FF72, FF73, FF74, FF75};
    } // namespace Undocumented

#endif

    namespace Boot {
        constexpr uint16_t BOOT = 0xFF50;
        constexpr uint16_t REGISTERS[] = {BOOT};
    } // namespace Boot
} // namespace Registers

namespace Bits {
    namespace Flags {
        constexpr uint8_t Z = 7;
        constexpr uint8_t N = 6;
        constexpr uint8_t H = 5;
        constexpr uint8_t C = 4;
    } // namespace Flags

    namespace Boot {
        constexpr uint8_t BOOT_ENABLE = 0;
    } // namespace Boot

    namespace Interrupts {
        constexpr uint8_t JOYPAD = 4;
        constexpr uint8_t SERIAL = 3;
        constexpr uint8_t TIMER = 2;
        constexpr uint8_t STAT = 1;
        constexpr uint8_t VBLANK = 0;
    } // namespace Interrupts

    namespace Joypad {
        namespace P1 {
            constexpr uint8_t SELECT_ACTION_BUTTONS = 5;
            constexpr uint8_t SELECT_DIRECTION_BUTTONS = 4;
            constexpr uint8_t INPUT_DOWN_OR_START = 3;
            constexpr uint8_t INPUT_UP_OR_SELECT = 2;
            constexpr uint8_t INPUT_LEFT_OR_B = 1;
            constexpr uint8_t INPUT_RIGHT_OR_A = 0;
        } // namespace P1
    } // namespace Joypad

    namespace Timers {
        namespace TAC {
            constexpr uint8_t ENABLE = 2;
            constexpr BitRange CLOCK_SELECTOR = {1, 0};
        } // namespace TAC
    } // namespace Timers

    namespace Video {
        namespace LCDC {
            constexpr uint8_t LCD_ENABLE = 7;
            constexpr uint8_t WIN_TILE_MAP = 6;
            constexpr uint8_t WIN_ENABLE = 5;
            constexpr uint8_t BG_WIN_TILE_DATA = 4;
            constexpr uint8_t BG_TILE_MAP = 3;
            constexpr uint8_t OBJ_SIZE = 2;
            constexpr uint8_t OBJ_ENABLE = 1;
            constexpr uint8_t BG_WIN_ENABLE = 0;
        } // namespace LCDC
        namespace STAT {
            constexpr uint8_t LYC_EQ_LY_INTERRUPT = 6;
            constexpr uint8_t OAM_INTERRUPT = 5;
            constexpr uint8_t VBLANK_INTERRUPT = 4;
            constexpr uint8_t HBLANK_INTERRUPT = 3;
            constexpr uint8_t LYC_EQ_LY = 2;
            constexpr uint8_t MODE_HIGH = 1;
            constexpr uint8_t MODE_LOW = 0;
        } // namespace STAT
#ifdef ENABLE_CGB
        namespace BCPS {
            constexpr uint8_t AUTO_INCREMENT = 7;
            constexpr BitRange ADDRESS = {5, 0};
        } // namespace BCPS
        namespace OCPS {
            constexpr uint8_t AUTO_INCREMENT = 7;
            constexpr BitRange ADDRESS = {5, 0};
        } // namespace OCPS
        namespace OPRI {
            constexpr uint8_t PRIORITY_MODE = 0;
        } // namespace OPRI
#endif
    } // namespace Video

    namespace Audio {
        namespace NR10 {
            constexpr BitRange PACE = {6, 4};
            constexpr uint8_t DIRECTION = 3;
            constexpr BitRange STEP = {2, 0};
        } // namespace NR10
        namespace NR11 {
            constexpr BitRange DUTY_CYCLE = {7, 6};
            constexpr BitRange INITIAL_LENGTH_TIMER = {5, 0};
        } // namespace NR11
        namespace NR12 {
            constexpr BitRange INITIAL_VOLUME = {7, 4};
            constexpr uint8_t ENVELOPE_DIRECTION = 3;
            constexpr BitRange SWEEP_PACE = {2, 0};
        } // namespace NR12
        namespace NR14 {
            constexpr uint8_t TRIGGER = 7;
            constexpr uint8_t LENGTH_ENABLE = 6;
            constexpr BitRange PERIOD = {2, 0};
        } // namespace NR14

        namespace NR21 {
            constexpr BitRange DUTY_CYCLE = {7, 6};
            constexpr BitRange INITIAL_LENGTH_TIMER = {5, 0};
        } // namespace NR21
        namespace NR22 {
            constexpr BitRange INITIAL_VOLUME = {7, 4};
            constexpr uint8_t ENVELOPE_DIRECTION = 3;
            constexpr BitRange SWEEP_PACE = {2, 0};
        } // namespace NR22
        namespace NR24 {
            constexpr uint8_t TRIGGER = 7;
            constexpr uint8_t LENGTH_ENABLE = 6;
            constexpr BitRange PERIOD = {2, 0};
        } // namespace NR24

        namespace NR30 {
            constexpr uint8_t DAC = 7;
        } // namespace NR30
        namespace NR32 {
            constexpr BitRange VOLUME = {6, 5};
        } // namespace NR32
        namespace NR34 {
            constexpr uint8_t TRIGGER = 7;
            constexpr uint8_t LENGTH_ENABLE = 6;
            constexpr BitRange PERIOD = {2, 0};
        } // namespace NR34

        namespace NR41 {
            constexpr BitRange INITIAL_LENGTH_TIMER = {5, 0};
        } // namespace NR41
        namespace NR42 {
            constexpr BitRange INITIAL_VOLUME = {7, 4};
            constexpr uint8_t ENVELOPE_DIRECTION = 3;
            constexpr BitRange SWEEP_PACE = {2, 0};
        } // namespace NR42
        namespace NR43 {
            constexpr BitRange CLOCK_SHIFT = {7, 4};
            constexpr uint8_t LFSR_WIDTH = 3;
            constexpr BitRange CLOCK_DIVIDER = {2, 0};
        } // namespace NR43
        namespace NR44 {
            constexpr uint8_t TRIGGER = 7;
            constexpr uint8_t LENGTH_ENABLE = 6;
        } // namespace NR44

        namespace NR50 {
            constexpr uint8_t VIN_LEFT = 7;
            constexpr BitRange VOLUME_LEFT = {6, 4};
            constexpr uint8_t VIN_RIGHT = 3;
            constexpr BitRange VOLUME_RIGHT = {2, 0};
        } // namespace NR50
        namespace NR51 {
            constexpr uint8_t CH4_LEFT = 7;
            constexpr uint8_t CH3_LEFT = 6;
            constexpr uint8_t CH2_LEFT = 5;
            constexpr uint8_t CH1_LEFT = 4;
            constexpr uint8_t CH4_RIGHT = 3;
            constexpr uint8_t CH3_RIGHT = 2;
            constexpr uint8_t CH2_RIGHT = 1;
            constexpr uint8_t CH1_RIGHT = 0;
        } // namespace NR51
        namespace NR52 {
            constexpr uint8_t AUDIO_ENABLE = 7;
            constexpr uint8_t CH4_ENABLE = 3;
            constexpr uint8_t CH3_ENABLE = 2;
            constexpr uint8_t CH2_ENABLE = 1;
            constexpr uint8_t CH1_ENABLE = 0;
        } // namespace NR52

    } // namespace Audio

    namespace OAM {
        namespace Attributes {
            constexpr uint8_t BG_OVER_OBJ = 7;
            constexpr uint8_t Y_FLIP = 6;
            constexpr uint8_t X_FLIP = 5;
            constexpr uint8_t DMG_PALETTE = 4;
#ifdef ENABLE_CGB
            constexpr uint8_t BANK = 3;
            constexpr BitRange CGB_PALETTE = {2, 0};
#endif
        } // namespace Attributes
    } // namespace OAM

#ifdef ENABLE_CGB
    namespace Background {
        namespace Attributes {
            constexpr uint8_t PRIORITY = 7;
            constexpr uint8_t Y_FLIP = 6;
            constexpr uint8_t X_FLIP = 5;
            constexpr uint8_t BANK = 3;
            constexpr BitRange PALETTE = {2, 0};
        } // namespace Attributes
    } // namespace Background
#endif

#ifdef ENABLE_CGB
    namespace Hdma {
        namespace HDMA5 {
            constexpr uint8_t HBLANK_TRANSFER = 7;
            constexpr BitRange TRANSFER_LENGTH = {6, 0};
        } // namespace HDMA5
    } // namespace Hdma
#endif

    namespace Serial {
        namespace SC {
            constexpr uint8_t TRANSFER_ENABLE = 7;
#ifdef ENABLE_CGB
            constexpr uint8_t CLOCK_SPEED = 1;
#endif
            constexpr uint8_t CLOCK_SELECT = 0;
        } // namespace SC
    } // namespace Serial

    namespace Rtc {
        namespace DH {
            constexpr uint8_t DAY_OVERFLOW = 7;
            constexpr uint8_t STOPPED = 6;
            constexpr uint8_t DAY = 0;
        } // namespace DH
    } // namespace Rtc

#ifdef ENABLE_CGB
    namespace SpeedSwitch {
        constexpr uint8_t CURRENT_SPEED = 7;
        constexpr uint8_t SPEED_SWITCH = 0;
    } // namespace SpeedSwitch
#endif

#ifdef ENABLE_CGB
    namespace Infrared {
        constexpr BitRange READ_ENABLE = {7, 6};
        constexpr uint8_t RECEIVING = 1;
        constexpr uint8_t EMITTING = 0;
    } // namespace Infrared
#endif
} // namespace Bits

namespace Bytes {
    namespace OAM {
        constexpr uint8_t ATTRIBUTES = 3;
        constexpr uint8_t TILE_NUMBER = 2;
        constexpr uint8_t X = 1;
        constexpr uint8_t Y = 0;
    } // namespace OAM
} // namespace Bytes

namespace Cartridge {
    namespace Header {
        namespace MemoryLayout {
            constexpr uint16_t START = 0x0000;
            constexpr uint16_t END = 0x014F;
            constexpr uint16_t SIZE = 0x0150;

            namespace ENTRY_POINT {
                constexpr uint16_t START = 0x0100;
                constexpr uint16_t END = 0x0103;
            } // namespace ENTRY_POINT
            namespace LOGO {
                constexpr uint16_t START = 0x0104;
                constexpr uint16_t END = 0x0133;
            } // namespace LOGO
            namespace TITLE {
                constexpr uint16_t START = 0x0134;
                constexpr uint16_t END = 0x0143;
            } // namespace TITLE
            namespace MANUFACTURER {
                constexpr uint16_t START = 0x013F;
                constexpr uint16_t END = 0x0142;
            } // namespace MANUFACTURER
            constexpr uint16_t CGB_FLAG = 0x0143;
            namespace NEW_LICENSEE {
                constexpr uint16_t START = 0x0144;
                constexpr uint16_t END = 0x0145;
            } // namespace NEW_LICENSEE
            constexpr uint16_t SGB_FLAG = 0x0146;
            constexpr uint16_t TYPE = 0x0147;
            constexpr uint16_t ROM_SIZE = 0x0148;
            constexpr uint16_t RAM_SIZE = 0x0149;
            constexpr uint16_t DESTINATION_CODE = 0x14A;
            constexpr uint16_t OLD_LICENSEE = 0x14B;
            constexpr uint16_t ROM_VERSION_NUMBER = 0x14C;
            constexpr uint16_t HEADER_CHECKSUM = 0x14D;
        } // namespace MemoryLayout

        namespace Mbc {
            constexpr uint32_t NO_MBC = 0x00;
            constexpr uint32_t MBC1 = 0x01;
            constexpr uint32_t MBC1_RAM = 0x02;
            constexpr uint32_t MBC1_RAM_BATTERY = 0x03;
            constexpr uint32_t MBC2 = 0x05;
            constexpr uint32_t MBC2_BATTERY = 0x06;
            constexpr uint32_t ROM_RAM = 0x08;
            constexpr uint32_t ROM_RAM_BATTERY = 0x09;
            constexpr uint32_t MBC3_TIMER_BATTERY = 0x0F;
            constexpr uint32_t MBC3_TIMER_RAM_BATTERY = 0x10;
            constexpr uint32_t MBC3 = 0x11;
            constexpr uint32_t MBC3_RAM = 0x12;
            constexpr uint32_t MBC3_RAM_BATTERY = 0x13;
            constexpr uint32_t MBC5 = 0x19;
            constexpr uint32_t MBC5_RAM = 0x1A;
            constexpr uint32_t MBC5_RAM_BATTERY = 0x1B;
            constexpr uint32_t MBC5_RUMBLE = 0x1C;
            constexpr uint32_t MBC5_RUMBLE_RAM = 0x1D;
            constexpr uint32_t MBC5_RUMBLE_RAM_BATTERY = 0x1E;
            constexpr uint32_t HUC3 = 0xFE;
            constexpr uint32_t HUC1 = 0xFF;
        } // namespace Mbc

        namespace Rom {
            constexpr uint32_t KB_32 = 0x0;
            constexpr uint32_t KB_64 = 0x1;
            constexpr uint32_t KB_128 = 0x2;
            constexpr uint32_t KB_256 = 0x3;
            constexpr uint32_t KB_512 = 0x4;
            constexpr uint32_t MB_1 = 0x5;
            constexpr uint32_t MB_2 = 0x6;
            constexpr uint32_t MB_4 = 0x7;
            constexpr uint32_t MB_8 = 0x8;
        } // namespace Rom

        namespace Ram {
            constexpr uint32_t NONE = 0x0;
            constexpr uint32_t KB_2 = 0x1; // unofficial
            constexpr uint32_t KB_8 = 0x2;
            constexpr uint32_t KB_32 = 0x3;
            constexpr uint32_t KB_128 = 0x4;
            constexpr uint32_t KB_64 = 0x5;
        } // namespace Ram

        namespace CgbFlag {
            constexpr uint8_t DMG_AND_CGB = 0x80;
            constexpr uint8_t CGB_ONLY = 0xC0;
        } // namespace CgbFlag

        constexpr uint8_t NINTENDO_LOGO[] = {0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83,
                                             0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
                                             0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
                                             0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E};
    } // namespace Header
} // namespace Cartridge

namespace Timers {
    constexpr uint8_t TAC_DIV_BITS_SELECTOR[] {9, 3, 5, 7};
}

constexpr double FPS = (double)Frequencies::CLOCK / Ppu::DOTS_PER_FRAME;
} // namespace Specs

#endif // SPECS_H