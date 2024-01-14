#ifndef PPU_H
#define PPU_H

#include "docboy/bootrom/macros.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "docboy/shared//macros.h"
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
    Ppu(Lcd& lcd, VideoIO& video, InterruptsIO& interrupts, VramBus::View<Device::Ppu> vramBus,
        OamBus::View<Device::Ppu> oamBus);

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

    // PPU helpers
    void turnOn();
    void turnOff();

    void tickStat();
    void updateStatIrq(bool irq);

    void tickWindow();

    // PPU states
    void oamScanEven();
    void oamScanOdd();
    void oamScanDone();

    void pixelTransferDummy0();
    void pixelTransferDiscard0();
    void pixelTransferDiscard0WX0SCX7();
    void pixelTransfer0();
    void pixelTransfer8();

    void hBlank();
    void hBlank453();
    void hBlank454();
    void hBlankLastLine();
    void hBlankLastLine454();
    void hBlankLastLine455();

    void vBlank();
    void vBlankLastLine();
    void vBlankLastLine454();

    // PPU states helpers
    void enterOamScan();
    void enterPixelTransfer();
    void enterHBlank();
    void enterVBlank();
    void enterNewFrame();

    void tickFetcher();
    void resetFetcher();

    template <uint8_t mode>
    void updateMode();

    void increaseLX();

    [[nodiscard]] bool isBgFifoReadyToBePopped() const;
    [[nodiscard]] bool isObjReadyToBeFetched() const;

    void eventuallySetupFetcherForWindow();
    void setupFetcherForWindow();

    template <uint8_t Mode>
    void readOamRegisters(uint16_t oamAddress);

    // Fetcher states
    void bgwinPrefetcherGetTile0();

    void bgPrefetcherGetTile0();
    void bgPrefetcherGetTile1();

    void winPrefetcherGetTile0();
    void winPrefetcherGetTile1();

    void objPrefetcherGetTile0();
    void objPrefetcherGetTile1();

    void bgPixelSliceFetcherGetTileDataLow0();
    void bgPixelSliceFetcherGetTileDataLow1();
    void bgPixelSliceFetcherGetTileDataHigh0();

    void winPixelSliceFetcherGetTileDataLow0();
    void winPixelSliceFetcherGetTileDataLow1();
    void winPixelSliceFetcherGetTileDataHigh0();

    void bgwinPixelSliceFetcherGetTileDataHigh1();
    void bgwinPixelSliceFetcherPush();

    void objPixelSliceFetcherGetTileDataLow0();
    void objPixelSliceFetcherGetTileDataLow1();
    void objPixelSliceFetcherGetTileDataHigh0();
    void objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo();

    // Fetcher states helpers
    void setupBgPixelSliceFetcherTileDataAddress();
    void setupWinPixelSliceFetcherTileDataAddress();

    void cacheBgWinFetch();
    void restoreBgWinFetch();

    Lcd& lcd;
    VideoIO& video;
    InterruptsIO& interrupts;
    VramBus::View<Device::Ppu> vram;
    OamBus::View<Device::Ppu> oam;

    TickSelector tickSelector {IF_BOOTROM_ELSE(&Ppu::oamScanEven, &Ppu::vBlankLastLine)};
    FetcherTickSelector fetcherTickSelector {&Ppu::bgPrefetcherGetTile0};

    bool on {};

    bool lastStatIrq {};

    uint16_t dots {IF_BOOTROM_ELSE(0, 395)}; // [0, 456)
    uint8_t LX {};                           // LX=X+8, therefore [0, 168)

    uint8_t BGP {IF_BOOTROM_ELSE(0, 0xFC)}; // video.BGP delayed by 1 t-cycle (~)
    uint8_t WX {};                          // video.WX delayed by 1 t-cycle

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

    struct {
        struct {
            uint8_t a {};
            uint8_t b {};
        } oam;
    } registers;

    // Oam Scan
    struct {
        uint8_t count {}; // [0, 10]
    } oamScan;

    // Pixel Transfer
    struct {
        struct {
            uint8_t toDiscard {}; // [0, 8)
            uint8_t discarded {}; // [0, 8)
        } initialSCX;
    } pixelTransfer;

    // Window
    struct {
        bool activeForFrame {};
        uint8_t WLY {UINT8_MAX}; // window line counter

        bool active {}; // currently rendering window

#ifdef ASSERTS_OR_DEBUGGER_ENABLED
        Vector<uint8_t, 20> lineTriggers {};
#endif
    } w;

    // Bg/Win Prefetcher
    struct {
        uint8_t LX {}; // [0, 256), advances 8 by 8
        uint8_t tilemapX {};
        uint8_t tilemapY {};

        struct {
            bool hasData {};
            uint8_t tileDataLow {};
            uint8_t tileDataHigh {};
        } interruptedFetch {};
    } bwf;

    // Window Prefetcher
    struct {
        uint8_t tilemapX {};
    } wf;

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