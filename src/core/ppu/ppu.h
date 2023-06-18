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

    template<class ProcessorImpl>
    class Processor {
    public:
        typedef void (ProcessorImpl::*TickHandler)();

        void tick() {
            auto* impl = static_cast<ProcessorImpl*>(this);
            TickHandler tickHandler = impl->tickHandlers[dots];
            (impl->*(tickHandler))();
            dots++;
        }

        void reset() {
            dots = 0;
        }

    protected:
        uint8_t dots;
    };

    struct BGPrefetcher : public Processor<BGPrefetcher> {
    friend class Processor<BGPrefetcher>;

    public:
        explicit BGPrefetcher(ILCDIO &lcdIo, IMemory &vram);

        void resetTile();

        [[nodiscard]] bool isTileDataAddressReady() const;
        [[nodiscard]] uint16_t getTileDataAddress() const;

        void advanceToNextTile();

    private:
        void tick_GetTile1();
        void tick_GetTile2();

        ILCDIO &lcdIo;
        IMemory &vram;

        TickHandler tickHandlers[2];

        // state
        uint8_t x8;

        // scratchpad
        uint8_t tilemapX;
        uint16_t tilemapAddr;
        uint8_t tileNumber;
        uint16_t tileAddr;

        // out
        uint16_t tileDataAddr;
    };


    class OBJPrefetcher : public Processor<OBJPrefetcher> {
    friend class Processor<OBJPrefetcher>;
    public:
         explicit OBJPrefetcher(ILCDIO &lcdIo, IMemory &oam);

        void setOAMEntry(const OAMEntry &oamEntry);

        [[nodiscard]] bool isTileDataAddressReady() const;
        [[nodiscard]] uint16_t getTileDataAddress() const;

    private:
        void tick_GetTile1();
        void tick_GetTile2();

        ILCDIO &lcdIo;
        IMemory &oam;

        TickHandler tickHandlers[2];

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

    class PixelSliceFetcher : public Processor<PixelSliceFetcher> {
    friend class Processor<PixelSliceFetcher>;
    public:
        explicit PixelSliceFetcher(IMemory &vram);

        void setTileDataAddress(uint16_t tileDataAddr);

        [[nodiscard]] bool isTileDataReady() const;

        [[nodiscard]] uint8_t getTileDataLow() const;
        [[nodiscard]] uint8_t getTileDataHigh() const;

    private:
        void tick_GetTileDataLow_1();
        void tick_GetTileDataLow_2();
        void tick_GetTileDataHigh_1();
        void tick_GetTileDataHigh_2();

        IMemory &vram;

        TickHandler tickHandlers[4];

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