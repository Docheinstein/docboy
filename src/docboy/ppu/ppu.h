#ifndef PPU_H
#define PPU_H

#include "docboy/bootrom/macros.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "docboy/debugger/macros.h"
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
    using VramBusView = AcquirableBusView<VramBus, AcquirableBusDevice::Ppu>;
    using OamBusView = AcquirableBusView<OamBus, AcquirableBusDevice::Ppu>;

    Ppu(Lcd& lcd, VideoIO& video, InterruptsIO& interrupts, VramBusView vramBus, OamBusView oamBus);

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

    struct OamScanEntry {
        uint8_t number; // [0, 40)
        uint8_t y;
        IF_ASSERTS_OR_DEBUGGER(uint8_t x);
    };

    void turnOn();
    void turnOff();

    // PPU states

    void oamScanEven();
    void oamScanOdd();
    void oamScanDone();

    void pixelTransferDummy0();
    void pixelTransferDiscard0();
    void pixelTransfer0();
    void pixelTransfer8();

    void hBlank();
    void hBlank454();
    void hBlankLastLine();
    void hBlankLastLine454();
    void hBlankLastLine455();

    void vBlank();
    void vBlankLastLine();
    void vBlankLastLine454();

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
    void tickStat();
    void updateStatIrq(bool irq);

    void tickFetcher();
    void resetFetcher();

    void enterOamScan();
    void enterPixelTransfer();
    void enterHBlank();
    void enterVBlank();
    void enterNewFrame();

    template <uint8_t mode>
    void updateMode();

    void increaseLX();

    [[nodiscard]] bool isBgFifoReadyToBePopped() const;
    [[nodiscard]] bool isObjReadyToBeFetched() const;
    [[nodiscard]] bool isWindowTriggering() const;
    void eventuallySetupFetcherForWindow();

    void cacheBgWinFetch();
    void restoreBgWinFetch();

    Lcd& lcd;
    VideoIO& video;
    InterruptsIO& interrupts;
    VramBusView vram;
    OamBusView oam;

    TickSelector tickSelector {IF_BOOTROM_ELSE(&Ppu::oamScanEven, &Ppu::vBlankLastLine)};
    FetcherTickSelector fetcherTickSelector {&Ppu::bgPrefetcherGetTile0};

    bool on {};

    bool lastStatIrq {};

    uint16_t dots {IF_BOOTROM_ELSE(0, 395)}; // [0, 456)
    uint8_t LX {};                           // LX=X+8, therefore [0, 168)

    FillQueue<BgPixel, 8> bgFifo {};
    Queue<ObjPixel, 8> objFifo {};

    Vector<OamScanEntry, 10> oamEntries[168] {};
    IF_ASSERTS(uint8_t oamEntriesCount {});
#ifdef ENABLE_DEBUGGER
    Vector<OamScanEntry, 10> scanlineOamEntries {}; // not cleared after oam scan
#endif

    bool isFetchingSprite {};

#ifdef ENABLE_DEBUGGER
    struct {
        uint16_t oamScan {};
        uint16_t pixelTransfer {};
        uint16_t hBlank {};
    } timings;
#endif

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
        struct {
            uint8_t toDiscard {}; // [0, 8)
            uint8_t discarded {}; // [0, 8)
        } initialSCX;
    } pixelTransfer;

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
        uint8_t WLY {UINT8_MAX}; // window line counter
        uint8_t WX {};           // triggered video.WX - 7
        bool active {};
        IF_ASSERTS(uint8_t lineTriggers {});
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

    IF_DEBUGGER(uint64_t cycles {});
};

#endif // PPU_H