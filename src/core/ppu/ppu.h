#ifndef PPU_H
#define PPU_H

#include <queue>
#include <cstdint>
#include "core/clock/clockable.h"
#include <optional>

class IMemory;
class ILCDIO;
class IInterruptsIO;
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


class PPU : public IPPU {
public:
    PPU(
        ILCD &lcd,
        ILCDIO &lcdIo,
        IInterruptsIO &interrupts,
        IMemory &vram,
        IMemory &oam);

    void tick() override;

protected:

    enum State {
        HBlank = 0,
        VBlank = 1,
        OAMScan = 2,
        PixelTransfer = 3
    };

    ILCD &lcd;
    ILCDIO &lcdIo;
    IInterruptsIO &interrupts;
    IMemory &vram;
    IMemory &oam;

    bool on;
    State state;

    // TODO: refactor for sure
    struct OAMEntry {
        uint8_t number;
        uint8_t x;
        uint8_t y;
    };
    std::vector<OAMEntry> oamEntries;
    struct {
        struct {
            uint8_t y;
        } oam;
    } scratchpad{};

    FIFO bgFifo;
    FIFO objFifo;

    uint8_t LX;

    // TODO: bad
    enum class FIFOType {
        Bg,
        Obj
    };

    // TODO: bad
    enum class FetcherState {
        Prefetcher,
        PixelSliceFetcher,
        Pushing
    };

    struct {
        FetcherState state;
//        uint8_t y;
        std::vector<OAMEntry> oamEntriesHit;
        FIFOType targetFifo;
    } fetcher;

    struct {
        uint8_t dots;
        struct {
            uint8_t tileDataLow;
            uint8_t tileDataHigh;
        } scratchpad;
        struct {
            uint16_t tileDataAddr;
        } in;
    } pixelSliceFetcher{};

    struct {
        uint8_t dots;
        uint8_t x8;
        struct {
            uint8_t tilemapX;
            uint16_t tilemapAddr;
            uint8_t tileNumber;
            uint16_t tileAddr;
        } scratchpad;
        struct {
            uint16_t tileDataAddr;
        } out;
    } bgPrefetcher{};

    struct {
        uint8_t dots;
        struct {
            uint8_t tileNumber;
            uint8_t oamFlags;
            uint16_t tileAddr;
        } scratchpad;
        struct {
            OAMEntry entry;
        } in;
        struct {
            uint16_t tileDataAddr;
        } out;
    } objPrefetcher{};

    uint32_t dots;
    uint64_t tCycles;

    void fetcherTick();
    void bgPrefetcherTick();
    void objPrefetcherTick();
    void pixelSliceFetcherTick();

    void fetcherClear();

    bool isFifoBlocked() const;
};



#endif // PPU_H