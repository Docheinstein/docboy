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
    struct OAMEntry {
        uint8_t number;
        uint8_t x;
        uint8_t y;
    };

    struct BGPrefetcher {
    public:
        explicit BGPrefetcher(ILCDIO &lcdIo, IMemory &vram);

        void tick();
        void reset();
        void resetTile();

        [[nodiscard]] bool isTileDataAddressReady() const;
        [[nodiscard]] uint16_t getTileDataAddress() const;

        void advanceToNextTile();

    private:
        ILCDIO &lcdIo;
        IMemory &vram;

        uint8_t dots;
        uint8_t x8;

        // scratchpad
        uint8_t tilemapX;
        uint16_t tilemapAddr;
        uint8_t tileNumber;
        uint16_t tileAddr;

        // out
        uint16_t tileDataAddr;
    };


    class OBJPrefetcher {
    public:
         explicit OBJPrefetcher(ILCDIO &lcdIo, IMemory &oam);

        void tick();
        void reset();

        void setOAMEntry(const OAMEntry &oamEntry);

        [[nodiscard]] bool isTileDataAddressReady() const;
        [[nodiscard]] uint16_t getTileDataAddress() const;

    private:
        ILCDIO &lcdIo;
        IMemory &oam;

        uint8_t dots;

        // scratchpad
        uint8_t tileNumber;

    public:
        // TODO: private
        uint8_t oamFlags;

    private:
        uint16_t tileAddr;

        // in
        OAMEntry entry;

        // out
        uint16_t tileDataAddr;
    };

    class PixelSliceFetcher {
    public:
        explicit PixelSliceFetcher(IMemory &vram);

        void tick();
        void reset();

        void setTileDataAddress(uint16_t tileDataAddr);

        [[nodiscard]] bool isTileDataReady() const;

        [[nodiscard]] uint8_t getTileDataLow() const;
        [[nodiscard]] uint8_t getTileDataHigh() const;

    private:
        IMemory &vram;

        uint8_t dots;

        // in
        uint16_t tileDataAddr;

        // out
        uint8_t tileDataLow;
        uint8_t tileDataHigh;
    };

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


    std::vector<OAMEntry> scanlineOamEntries;
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
        std::vector<OAMEntry> oamEntriesHit;
        FIFOType targetFifo;
    } fetcher;


    BGPrefetcher bgPrefetcher;
    OBJPrefetcher objPrefetcher;
    PixelSliceFetcher pixelSliceFetcher;

    uint32_t dots;
    uint64_t tCycles;

    void fetcherTick();
    void fetcherClear();

    bool isFifoBlocked() const;
};



#endif // PPU_H