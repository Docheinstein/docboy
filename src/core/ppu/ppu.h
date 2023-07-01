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


class PPU : public IPPU {
public:
    struct Pixel {
        uint8_t color;
        uint8_t palette;
        uint8_t priority;
    };

    using PixelFIFO = std::deque<Pixel>;

    PPU(
        ILCD &lcd,
        ILCDIO &lcdIo,
        IInterruptsIO &interrupts,
        IMemory &vram,
        IMemory &oam);

    void tick() override;

protected:
    typedef void (PPU::*TickHandler)();

    struct OAMEntry {
        uint8_t number;
        uint8_t x;
        uint8_t y;
    };

    class Fetcher {
    public:
        Fetcher(ILCDIO &lcdIo, IMemory &vram, IMemory &oam,
                PixelFIFO &bgFifo, PixelFIFO &objFifo);

        void tick();
        void reset();

        // TODO: don't like
        bool isFetchingSprite() const;

        void setOAMEntriesHit(const std::vector<OAMEntry> &entries);

    private:
        // base class for BGPrefetcher, OBJPrefetcher, PixelSliceFetcher
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

        enum class FIFOType {
            Bg,
            Obj
        };
        enum class State {
            Prefetcher,
            PixelSliceFetcher,
            Pushing
        };

        PixelFIFO &bgFifo;
        PixelFIFO &objFifo;

        State state;
        std::vector<OAMEntry> oamEntriesHit;
        FIFOType targetFifo;

        BGPrefetcher bgPrefetcher;
        OBJPrefetcher objPrefetcher;
        PixelSliceFetcher pixelSliceFetcher;
    };

    enum State {
        HBlank = 0,
        VBlank = 1,
        OAMScan = 2,
        PixelTransfer = 3
    };

    void tick_HBlank();
    void afterTick_HBlank();

    void tick_VBlank();
    void afterTick_VBlank();

    void tick_OAMScan();
    void afterTick_OAMScan();

    void tick_PixelTransfer();
    void afterTick_PixelTransfer();

    // TODO: don't like
    void enterHBlank();
    void enterVBlank();
    void enterOAMScan();
    void enterPixelTransfer();

    void setState(PPU::State s);
    void setLY(uint8_t LY);

    uint8_t LY() const;

    bool isFifoBlocked() const;

    void turnOn();
    void turnOff();

    ILCD &lcd;
    ILCDIO &lcdIo;
    IInterruptsIO &interrupts;
    IMemory &vram;
    IMemory &oam;

    TickHandler tickHandlers[4];
    TickHandler afterTickHandlers[4];

    bool on;
    State state;

    std::vector<OAMEntry> scanlineOamEntries;
    struct {
        struct {
            uint8_t y;
        } oamScan;
        struct {
            bool pixelPushed;
        } pixelTransfer;
    } scratchpad{};

    Fetcher fetcher;

    PixelFIFO bgFifo;
    PixelFIFO objFifo;

    uint8_t LX;

    uint32_t dots;
    uint64_t tCycles;
};



#endif // PPU_H