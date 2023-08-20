#ifndef PPU_H
#define PPU_H

#include "core/clock/clockable.h"
#include "core/state/processor.h"
#include <cstdint>
#include <optional>
#include <queue>

// TODO: find a nicer approach to debug PPU internal classes
#ifdef ENABLE_DEBUGGER
#define private public
#define protected public
#endif

class IMemory;
class ILCDIO;
class IInterruptsIO;
class ILCD;

class IPPU : public IClockable, public IStateProcessor {};

class PPU : public IPPU {
public:
    PPU(ILCD& lcd, ILCDIO& lcdIo, IInterruptsIO& interrupts, IMemory& vram, IMemory& oam);

    void tick() override;

    void loadState(IReadableState& state) override;
    void saveState(IWritableState& state) override;

protected:
    typedef void (PPU::*TickHandler)();

    struct OAMEntryFetchInfo {
        uint8_t number {}; // oam entry number, between 0 and 39
        uint8_t x {};      // byte 1
        uint8_t y {};      // byte 0
    };

    struct OAMEntry : OAMEntryFetchInfo {
        OAMEntry() :
            OAMEntryFetchInfo() {
        }
        explicit OAMEntry(const OAMEntryFetchInfo& other);
        OAMEntry& operator=(const OAMEntryFetchInfo& other) {
            number = other.number;
            x = other.x;
            y = other.y;
            return *this;
        }

        uint8_t flags {}; // byte 3
    };

    struct Pixel {
        explicit Pixel(uint8_t colorIndex);

        uint8_t colorIndex {}; // color value between 0 and 3
    };

    using BGPixel = Pixel;

    struct OBJPixel : Pixel {
        explicit OBJPixel(uint8_t colorIndex, const OAMEntry& entry);

        uint8_t palette {};   // palette
        uint8_t priority {};  // oam number
        uint8_t x {};         // oam x
        uint8_t bgOverObj {}; // render bg over obj
    };

    using BGPixelFIFO = std::deque<BGPixel>;
    using OBJPixelFIFO = std::deque<OBJPixel>;

    class Fetcher {
    public:
        Fetcher(ILCDIO& lcdIo, IMemory& vram, IMemory& oam, BGPixelFIFO& bgFifo, OBJPixelFIFO& objFifo);

        void tick();
        void reset();

        // TODO: don't like
        [[nodiscard]] bool isFetchingSprite() const;

        void clearOAMEntriesHit();
        void addOAMEntryHit(const OAMEntryFetchInfo& entry);

    private:
        // base class for BGPrefetcher, OBJPrefetcher, PixelSliceFetcher
        template <class ProcessorImpl>
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
            explicit BGPrefetcher(ILCDIO& lcdIo, IMemory& vram);

            void resetTile();

            [[nodiscard]] bool isTileDataAddressReady() const;
            [[nodiscard]] uint16_t getTileDataAddress() const;

            // TODO: bad design
            [[nodiscard]] uint8_t getPixelsToDiscardCount() const;

            void advanceToNextTile();

        private:
            enum class FetchType { Background, Window };

            void tick_GetTile1();
            void tick_GetTile2();

            ILCDIO& lcdIo;
            IMemory& vram;

            TickHandler tickHandlers[2];

            // state
            uint8_t x8;

            // scratchpad
            FetchType fetchType;
            uint8_t tilemapX;
            uint16_t tilemapAddr;
            uint8_t tileNumber;
            uint16_t tileAddr;

            uint8_t pixelsToDiscard; // TODO: refactor this

            // out
            uint16_t tileDataAddr;
        };

        class OBJPrefetcher : public Processor<OBJPrefetcher> {
            friend class Processor<OBJPrefetcher>;

        public:
            explicit OBJPrefetcher(ILCDIO& lcdIo, IMemory& oam);

            void setOAMEntryFetchInfo(const OAMEntryFetchInfo& oamEntry);
            [[nodiscard]] OAMEntry getOAMEntry() const;

            [[nodiscard]] bool areTileDataAddressAndFlagsReady() const;

            [[nodiscard]] uint16_t getTileDataAddress() const;

        private:
            void tick_GetTile1();
            void tick_GetTile2();

            ILCDIO& lcdIo;
            IMemory& oam;

            TickHandler tickHandlers[2];

            // scratchpad
            uint8_t tileNumber;

            // out
            uint16_t tileDataAddr;
            OAMEntry entry;
        };

        class PixelSliceFetcher : public Processor<PixelSliceFetcher> {
            friend class Processor<PixelSliceFetcher>;

        public:
            explicit PixelSliceFetcher(IMemory& vram);

            void setTileDataAddress(uint16_t tileDataAddr);

            [[nodiscard]] bool isTileDataReady() const;

            [[nodiscard]] uint8_t getTileDataLow() const;
            [[nodiscard]] uint8_t getTileDataHigh() const;

        private:
            void tick_GetTileDataLow_1();
            void tick_GetTileDataLow_2();
            void tick_GetTileDataHigh_1();
            void tick_GetTileDataHigh_2();

            IMemory& vram;

            TickHandler tickHandlers[4];

            // in
            uint16_t tileDataAddr;

            // out
            uint8_t tileDataLow;
            uint8_t tileDataHigh;
        };

        enum class FIFOType { Bg, Obj };
        enum class State { Prefetcher, PixelSliceFetcher, Pushing };

        BGPixelFIFO& bgFifo;
        OBJPixelFIFO& objFifo;

        State state;
        std::vector<OAMEntryFetchInfo> oamEntriesHit;
        FIFOType targetFifo;

        int fetchNumber;

        BGPrefetcher bgPrefetcher;
        OBJPrefetcher objPrefetcher;
        PixelSliceFetcher pixelSliceFetcher;

        uint8_t dots;
    };

    enum State { HBlank = 0, VBlank = 1, OAMScan = 2, PixelTransfer = 3 };

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

    [[nodiscard]] uint8_t LY() const;

    [[nodiscard]] bool isFifoBlocked() const;

    void turnOn();
    void turnOff();

    ILCD& lcd;
    ILCDIO& lcdIo;
    IInterruptsIO& interrupts;
    IMemory& vram;
    IMemory& oam;

    TickHandler tickHandlers[4];
    TickHandler afterTickHandlers[4];

    bool on;
    State state;

    std::vector<OAMEntryFetchInfo> scanlineOamEntries;
    struct {
        struct {
            uint8_t y;
        } oamScan;
        struct {
            bool firstOamEntriesCheck {true};
            bool pixelPushed;
        } pixelTransfer;
    } scratchpad {};

    Fetcher fetcher;

    BGPixelFIFO bgFifo;
    OBJPixelFIFO objFifo;

    uint8_t LX; // LX=X+8, therefore goes from 0 to 168

    uint32_t dots;
    uint64_t tCycles;
};

// TODO: find a nicer approach to debug PPU internal classes
#ifdef ENABLE_DEBUGGER
#undef private
#define private private
#undef protected
#define protected protected
#endif

#endif // PPU_H