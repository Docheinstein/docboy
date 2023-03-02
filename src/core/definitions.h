#ifndef DEFINITIONS_H
#define DEFINITIONS_H


#include <string>

namespace Frequencies {
    constexpr uint32_t Cpu = 4194304 / 4; // m-cycles
    constexpr uint32_t DIV = 16384;
    constexpr uint32_t TAC_SELECTOR[4] = {4096, 262144, 65536, 16384};
}

namespace MemoryMap {
    namespace ROM0 {
        constexpr uint16_t START  = 0x0000;
        constexpr uint16_t END    = 0x3FFF;
    }
    namespace ROM1 {
        constexpr uint16_t START  = 0x4000;
        constexpr uint16_t END    = 0x7FFF;
    }
    namespace VRAM {
        constexpr uint16_t START  = 0x8000;
        constexpr uint16_t END    = 0x9FFF;
    }
    namespace RAM {
        constexpr uint16_t START  = 0xA000;
        constexpr uint16_t END    = 0xBFFF;
    }
    namespace WRAM1 {
        constexpr uint16_t START  = 0xC000;
        constexpr uint16_t END    = 0xCFFF;
    }
    namespace WRAM2 {
        constexpr uint16_t START  = 0xD000;
        constexpr uint16_t END    = 0xDFFF;
    }
    namespace OAM {
        constexpr uint16_t START  = 0xFE00;
        constexpr uint16_t END    = 0xFE9F;
    }
    namespace IO {
        constexpr uint16_t START  = 0xFF00;
        constexpr uint16_t END    = 0xFF7F;

        constexpr uint16_t SB = 0xFF01;
        constexpr uint16_t SC = 0xFF02;
        constexpr uint16_t DIV = 0xFF04;
        constexpr uint16_t TIMA = 0xFF05;
        constexpr uint16_t TMA = 0xFF06;
        constexpr uint16_t TAC = 0xFF07;
        constexpr uint16_t IF = 0xFF0F;
    }
    namespace HRAM {
        constexpr uint16_t START  = 0xFF80;
        constexpr uint16_t END    = 0xFFFE;
    }

    constexpr uint16_t IE = 0xFFFF;
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

    namespace IO {
        namespace TAC {
            constexpr uint8_t ENABLE = 2;
        }
    }
}


#endif // DEFINITIONS_H