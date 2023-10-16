#ifndef PPU_H
#define PPU_H

#include "docboy/debugger/macros.h"
#include "docboy/memory/fwd/oamfwd.h"
#include "docboy/memory/fwd/vramfwd.h"
#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/fillqueue.hpp"
#include "utils/queue.hpp"
#include "utils/vector.hpp"

#include <vector>

class Lcd;
class VideoIO;
class InterruptsIO;
class Parcel;

class Ppu {
    DEBUGGABLE_CLASS()

public:
    Ppu(Lcd& lcd, VideoIO& video, InterruptsIO& interrupts, Vram& vram, Oam& oam);

    void tick();

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

private:
    using TickSelector = void (Ppu::*)();
    using FetcherTickSelector = void (Ppu::*)();
    using PixelColorIndex = uint8_t;

    struct BgPixel {
        PixelColorIndex colorIndex;
    };

    struct ObjPixel {
        PixelColorIndex colorIndex;
        uint8_t attributes;
        uint8_t number; // TODO: probably can be removed since oam scan is ordered by number
        uint8_t x;
    };

    enum class Phase : uint8_t {
        HBlank = 0,
        VBlank = 1,
        OamScan = 2,
        PixelTransfer = 3,
    };

    struct OamScanEntry {
        uint8_t number; // [0, 40)
        uint8_t y;
        ASSERTS_OR_DEBUGGER_ONLY(uint8_t x);
    };

    void turnOn();
    void turnOff();

    // PPU states
    void oamScan0();
    void oamScan1();
    void oamScanCompleted();
    template <bool Bg>
    void pixelTransferDummy0();
    void pixelTransferDiscard0();
    void pixelTransfer0();
    void pixelTransfer8();
    void hBlank();
    void vBlank();

    // Fetcher states
    void bgwinPrefetcherGetTile0();

    void bgPrefetcherGetTile0();
    void bgPrefetcherGetTile1();

    void winPrefetcherGetTile0();
    void winPrefetcherGetTile1();

    void objPrefetcherGetTile0();
    void objPrefetcherGetTile1();

    void bgwinPixelSliceFetcherGetTileDataLow0();
    void bgwinPixelSliceFetcherGetTileDataLow1();
    void bgwinPixelSliceFetcherGetTileDataHigh0();
    void bgwinPixelSliceFetcherGetTileDataHigh1();
    void bgwinPixelSliceFetcherPush();

    void objPixelSliceFetcherGetTileDataLow0();
    void objPixelSliceFetcherGetTileDataLow1();
    void objPixelSliceFetcherGetTileDataHigh0();
    void objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo();

    // Misc
    void tickFetcher();
    void resetFetcher();

    void enterOamScan();
    void enterPixelTransfer();
    void enterHBlank();
    void enterVBlank();
    void enterNewFrame();

    template <Phase P>
    void updateStat();

    void writeLY(uint8_t LY);

    [[nodiscard]] bool isPixelTransferBlocked() const;

    void cacheInterruptedBgWinFetch();
    void restoreInterruptedBgWinFetch();

    Lcd& lcd;
    VideoIO& video;
    InterruptsIO& interrupts;
    Vram& vram;
    Oam& oam;

    TickSelector tickSelector {&Ppu::oamScan0};
    FetcherTickSelector fetcherTickSelector {&Ppu::bgPrefetcherGetTile0};

    bool on {};

    uint16_t dots {}; // [0, 456)
    uint8_t LX {};    // LX=X+8, therefore [0, 168)

    FillQueue<BgPixel, 8> bgFifo {};
    Queue<ObjPixel, 8> objFifo {};

    Vector<OamScanEntry, 10> oamEntries[168] {};
    ASSERTS_ONLY(uint8_t oamEntriesCount {});
#ifdef ENABLE_DEBUGGER
    Vector<OamScanEntry, 10> scanlineOamEntries {}; // not cleared after oam scan
#endif

    bool isFetchingSprite {};

    // Scratchpads

    // Oam Scan
    struct {
        uint8_t count {}; // [0, 10]
        struct {
            uint8_t number {};
            uint8_t y {};
        } entry;
    } oamScan;

    struct {
        uint8_t SCXmod8 {};         // [0, 8)
        uint8_t discardedPixels {}; // [0, 8)
    } bgPixelTransfer;

    // Bg/Win Prefetcher
    struct {
        uint8_t LX {}; // [0, 256), advances 8 by 8
        uint8_t tilemapX {};

        struct {
            uint8_t tileDataLow {};
            uint8_t tileDataHigh {};
            bool hasData {};
        } interruptedFetch {};
    } bwf;

    // Window
    struct {
        uint8_t WLY {};
        bool shown {};
    } w;

    // Obj Prefetcher
    struct {
        OamScanEntry entry {}; // hit obj
        uint8_t tileNumber {}; // [0, 256)
        uint8_t attributes {};
    } of;

    // Pixel Slice Fetcher
    struct {
        uint16_t vTileDataAddress {};
        uint8_t tileDataLow {};
        uint8_t tileDataHigh {};
    } psf;

    DEBUGGER_ONLY(uint64_t cycles {});

    ASSERTS_ONLY(bool firstFetchWasBg {});
};

#endif // PPU_H