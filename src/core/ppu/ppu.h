#ifndef PPU_H
#define PPU_H

#include <queue>
#include <cstdint>
#include "core/clock/clockable.h"

class IMemory;
class IIO;
class ILCD;

using IPPU = IClockable;

struct FIFO {
public:
    struct Pixel {
        uint8_t color;
        uint8_t palette;
        uint8_t priority;
    };

    std::deque<Pixel> pixels;
};


class PPU : public virtual IPPU {
public:
    PPU(ILCD &lcd, IMemory &vram, IMemory &oam, IIO &io);

    void tick() override;

protected:

    enum State {
        HBlank = 0,
        VBlank = 1,
        OAMScan = 2,
        PixelTransfer = 3
    };

    ILCD &lcd;
    IMemory &vram;
    IMemory &oam;
    IIO &io;

    FIFO bgFifo;
    FIFO objFifo;

    State state;

    uint8_t transferredPixels;

    struct {
        uint8_t dots;
        uint8_t x8;
        uint8_t y;
        struct {
            uint8_t tilemapX;
//            uint8_t y;
            uint8_t tileNumber;
            uint16_t tilemapAddr;
            uint16_t tileAddr;
            uint16_t tileDataAddr;
            uint8_t tileDataLow;
            uint8_t tileDataHigh;
        } scratchpad;
    } fetcher;

    uint32_t dots;
    uint64_t tCycles;

    void fetcherTick();
    void fetcherClear();
};



#endif // PPU_H