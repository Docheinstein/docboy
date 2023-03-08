#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <string>

namespace Specs {
    constexpr uint32_t FREQUENCY = 4194304;

    namespace Display {
        constexpr uint32_t WIDTH = 160;
        constexpr uint32_t HEIGHT = 144;
    }

    namespace PPU {
        constexpr uint32_t FREQUENCY = Specs::FREQUENCY;
        constexpr uint32_t FRAME_DURATION = 70224;
    }

    namespace CPU {
        constexpr uint32_t FREQUENCY = Specs::FREQUENCY / 4;
        constexpr uint32_t DIV_FREQUENCY = 16384;
        constexpr uint32_t TAC_FREQUENCY[] = {4096, 262144, 65536, 16384};
    }
}

namespace MemoryMap {
    namespace ROM0 {
        constexpr uint16_t START  = 0x0000;
        constexpr uint16_t END    = 0x3FFF;
        constexpr uint16_t SIZE    = END - START + 1;
    }
    namespace ROM1 {
        constexpr uint16_t START  = 0x4000;
        constexpr uint16_t END    = 0x7FFF;
        constexpr uint16_t SIZE    = END - START + 1;
    }
    namespace VRAM {
        constexpr uint16_t START  = 0x8000;
        constexpr uint16_t END    = 0x9FFF;
        constexpr uint16_t SIZE    = END - START + 1;
    }
    namespace RAM {
        constexpr uint16_t START  = 0xA000;
        constexpr uint16_t END    = 0xBFFF;
        constexpr uint16_t SIZE    = END - START + 1;
    }
    namespace WRAM1 {
        constexpr uint16_t START  = 0xC000;
        constexpr uint16_t END    = 0xCFFF;
        constexpr uint16_t SIZE    = END - START + 1;
    }
    namespace WRAM2 {
        constexpr uint16_t START  = 0xD000;
        constexpr uint16_t END    = 0xDFFF;
        constexpr uint16_t SIZE    = END - START + 1;
    }
    namespace ECHO_RAM {
        constexpr uint16_t START  = 0xE000;
        constexpr uint16_t END    = 0xFDFF;
        constexpr uint16_t SIZE    = END - START + 1;
    }
    namespace OAM {
        constexpr uint16_t START  = 0xFE00;
        constexpr uint16_t END    = 0xFE9F;
        constexpr uint16_t SIZE    = END - START + 1;
    }
    namespace NOT_USABLE {
        constexpr uint16_t START  = 0xFEA0;
        constexpr uint16_t END    = 0xFEFF;
        constexpr uint16_t SIZE    = END - START + 1;
    }

    namespace IO {
        constexpr uint16_t START  = 0xFF00;
        constexpr uint16_t END    = 0xFF7F;
        constexpr uint16_t SIZE    = END - START + 1;
    }

    namespace HRAM {
        constexpr uint16_t START  = 0xFF80;
        constexpr uint16_t END    = 0xFFFE;
        constexpr uint16_t SIZE    = END - START + 1;
    }

    constexpr uint16_t IE = 0xFFFF;
}

namespace Registers {
    namespace Joypad {
        constexpr uint16_t P1 = 0xFF00;
        constexpr uint16_t REGISTERS[] = { P1 };
    }

    namespace Serial {
        constexpr uint16_t SB = 0xFF01;
        constexpr uint16_t SC = 0xFF02;
        constexpr uint16_t REGISTERS[] = { SB, SC };
    }

    namespace Timers {
        constexpr uint16_t DIV = 0xFF04;
        constexpr uint16_t TIMA = 0xFF05;
        constexpr uint16_t TMA = 0xFF06;
        constexpr uint16_t TAC = 0xFF07;
        constexpr uint16_t REGISTERS[] = { DIV, TIMA, TMA, TAC };
    }

    namespace Interrupts {
        constexpr uint16_t IF = 0xFF0F;
        constexpr uint16_t IE = 0xFFFF;
        constexpr uint16_t REGISTERS[] = { IF, IE };
    }

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
        constexpr uint16_t REGISTERS[] = {
                NR10, NR11, NR12, NR13,
                NR14, NR21, NR22, NR23,
                NR24, NR30, NR31, NR32,
                NR33, NR34, NR41, NR42,
                NR43, NR44, NR50, NR51,
                NR52, WAVE0, WAVE1, WAVE2,
                WAVE3, WAVE4, WAVE5, WAVE6,
                WAVE7, WAVE8, WAVE9, WAVEA,
                WAVEB, WAVEC, WAVED, WAVEE, WAVEF
        };
    }

    namespace LCD {
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
        constexpr uint16_t REGISTERS[] = {
                LCDC, STAT, SCY, SCX,
                LY, LYC, DMA, BGP,
                OBP0, OBP1, WY, WX,
        };
    }

    namespace Boot {
        constexpr uint16_t BOOTROM = 0xFF50;
        constexpr uint16_t REGISTERS[] = { BOOTROM };
    }
}

namespace Bits {
    namespace Flags {
        constexpr uint8_t Z = 7;
        constexpr uint8_t N = 6;
        constexpr uint8_t H = 5;
        constexpr uint8_t C = 4;
    }

    namespace Interrupts {
        constexpr uint8_t JOYPAD = 4;
        constexpr uint8_t SERIAL = 3;
        constexpr uint8_t TIMER = 2;
        constexpr uint8_t STAT = 1;
        constexpr uint8_t VBLANK = 0;
    }

    namespace Registers {
        namespace Timers {
            namespace TAC {
                constexpr uint8_t ENABLE = 2;
            }
        }

        namespace LCD {
            namespace LCDC {
                constexpr uint8_t LCD_ENABLE = 7;
                constexpr uint8_t WIN_TILE_MAP = 6;
                constexpr uint8_t WIN_ENABLE = 5;
                constexpr uint8_t BG_WIN_TILE_DATA = 4;
                constexpr uint8_t BG_TILE_MAP = 3;
                constexpr uint8_t OBJ_SIZE = 2;
                constexpr uint8_t OBJ_ENABLE = 1;
                constexpr uint8_t BG_WIN_ENABLE = 0;
            }
        }
    }
}

#endif // DEFINITIONS_H