#include "ppu.h"

#include <iostream>

#include "docboy/interrupts/interrupts.h"
#include "docboy/lcd/lcd.h"
#include "docboy/memory/memory.hpp"
#include "docboy/ppu/video.h"
#include "pixelmap.h"
#include "utils/casts.hpp"
#include "utils/macros.h"
#include "utils/mathematics.h"
#include "utils/traits.h"

using namespace Specs::Bits::Video::LCDC;
using namespace Specs::Bits::Video::STAT;
using namespace Specs::Bits::OAM::Attributes;

using namespace Specs::Ppu::Modes;

namespace {
constexpr uint8_t TILE_WIDTH = 8;
constexpr uint8_t TILE_HEIGHT = 8;
constexpr uint8_t TILE_DOUBLE_HEIGHT = 16;
constexpr uint8_t TILE_BYTES = 16;
constexpr uint8_t TILE_ROW_BYTES = 2;

constexpr uint8_t TILEMAP_WIDTH = 32;
constexpr uint8_t TILEMAP_HEIGHT = 32;
constexpr uint8_t TILEMAP_CELL_BYTES = 1;

constexpr uint8_t OAM_ENTRY_BYTES = 4;

constexpr uint8_t OBJ_COLOR_INDEX_TRANSPARENT = 0;

constexpr uint8_t NUMBER_OF_COLORS = 4;
constexpr uint8_t BITS_PER_PIXEL = 2;

constexpr uint8_t DUMMY_PIXEL = 0xFF;
constexpr uint8_t DUMMY_TILE[8] {DUMMY_PIXEL, DUMMY_PIXEL, DUMMY_PIXEL, DUMMY_PIXEL,
                                 DUMMY_PIXEL, DUMMY_PIXEL, DUMMY_PIXEL, DUMMY_PIXEL};

inline bool isObjOpaque(const uint8_t colorIndex) {
    return colorIndex != OBJ_COLOR_INDEX_TRANSPARENT;
}

inline bool isObjTransparent(const uint8_t colorIndex) {
    return colorIndex == OBJ_COLOR_INDEX_TRANSPARENT;
}

inline uint8_t resolveColor(const uint8_t colorIndex, const uint8_t palette) {
    check(colorIndex < NUMBER_OF_COLORS);
    return keep_bits<BITS_PER_PIXEL>(palette >> (BITS_PER_PIXEL * colorIndex));
}
} // namespace

const Ppu::TickSelector Ppu::TICK_SELECTORS[] = {
    &Ppu::oamScanEven,
    &Ppu::oamScanOdd,
    &Ppu::oamScanDone,
    &Ppu::oamScanAfterTurnOn,
    &Ppu::pixelTransferDummy0,
    &Ppu::pixelTransferDiscard0,
    &Ppu::pixelTransferDiscard0WX0SCX7,
    &Ppu::pixelTransfer0,
    &Ppu::pixelTransfer8,
    &Ppu::hBlank,
    &Ppu::hBlank453,
    &Ppu::hBlank454,
    &Ppu::hBlank455,
    &Ppu::hBlankLastLine,
    &Ppu::hBlankLastLine454,
    &Ppu::hBlankLastLine455,
    &Ppu::vBlank,
    &Ppu::vBlank454,
    &Ppu::vBlankLastLine,
    &Ppu::vBlankLastLine2,
    &Ppu::vBlankLastLine7,
    &Ppu::vBlankLastLine454,
};

const Ppu::FetcherTickSelector Ppu::FETCHER_TICK_SELECTORS[] = {
    &Ppu::bgwinPrefetcherGetTile0,
    &Ppu::bgPrefetcherGetTile0,
    &Ppu::bgPrefetcherGetTile1,
    &Ppu::bgPixelSliceFetcherGetTileDataLow0,
    &Ppu::bgPixelSliceFetcherGetTileDataLow1,
    &Ppu::bgPixelSliceFetcherGetTileDataHigh0,
    &Ppu::winPrefetcherActivating,
    &Ppu::winPrefetcherGetTile0,
    &Ppu::winPrefetcherGetTile1,
    &Ppu::winPixelSliceFetcherGetTileDataLow0,
    &Ppu::winPixelSliceFetcherGetTileDataLow1,
    &Ppu::winPixelSliceFetcherGetTileDataHigh0,
    &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1,
    &Ppu::bgwinPixelSliceFetcherPush,
    &Ppu::objPrefetcherGetTile0,
    &Ppu::objPrefetcherGetTile1,
    &Ppu::objPixelSliceFetcherGetTileDataLow0,
    &Ppu::objPixelSliceFetcherGetTileDataLow1,
    &Ppu::objPixelSliceFetcherGetTileDataHigh0,
    &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo};

Ppu::Ppu(Lcd& lcd, VideoIO& video, InterruptsIO& interrupts, VramBus::View<Device::Ppu> vramBus,
         OamBus::View<Device::Ppu> oamBus) :
    lcd(lcd),
    video(video),
    interrupts(interrupts),
    vram(vramBus),
    oam(oamBus) {
}

void Ppu::tick() {
    // Handle turn on/turn off
    if (on) {
        if (!test_bit<LCD_ENABLE>(video.LCDC)) {
            turnOff();
            return;
        }
    } else {
        if (test_bit<LCD_ENABLE>(video.LCDC)) {
            turnOn();
        } else {
            return;
        }
    }

    // Tick PPU by one dot
    (this->*(tickSelector))();

    // There is 1 T-cycle delay between video.BGP and the BGP value the PPU sees.
    // During this T-Cycle, the PPU sees the last BGP ORed with the new one.
    // Therefore, we store the last BGP value here so that we can use (video.BGP | BGP)
    // for resolving BG color.
    // [mealybug/m3_bgp_change]
    BGP = video.BGP;

    // It seems that there is 1 T-cycle delay between video.WX and the WX value the PPU sees.
    // Therefore, we store the last WX here and we use it for the next T-cycle.
    // (TODO: Still not sure about this)
    // [mealybug/m3_wx_5_change]
    WX = video.WX;

    // This is needed for:
    // 1) LCDC.WIN_ENABLE, because it seems that the t-cycle window is turned on
    //    (and it was off), window is activated also for LX == WX + 1, not only for LX == WX.
    //    [mealybug/m3_lcdc_win_en_change_multiple_wx]
    // 2) LCDC.BG_WIN_ENABLE, because it seems that there is a 1 T-cycle delay for this bit
    //    [mealybug/m3_lcdc_bg_en_change]
    lastLCDC = video.LCDC;

    // Update STAT's LYC_EQ_LY and eventually raise STAT interrupt
    tickStat();

    // Check for window activation (the state holds for entire frame).
    // The activation conditions are checked every dot,
    // (not only during pixel transfer or at LY == WX).
    // This is also reflected in the fact that window glitches for
    // the rest of the frame by pushing 00 pixels if WX == LX during the fetched
    // push stage if at some point in the entire frame the condition
    // WY = LY was satisfied.
    tickWindow();

    check(dots < 456);

    IF_DEBUGGER(++cycles);
}

// ------- PPU helpers -------

void Ppu::turnOn() {
    check(!on);
    check(video.LY == 0);

    on = true;

    // STAT's LYC_EQ_LY is updated properly (but interrupt is not raised)
    set_bit<LYC_EQ_LY>(video.STAT, isLYCEqLY());
}

void Ppu::turnOff() {
    check(on);
    on = false;
    dots = 0;
    video.LY = 0;
    lcd.resetCursor();

    // Clear oam entries eventually still there
    for (uint32_t i = 0; i < array_size(oamEntries); i++)
        oamEntries[i].clear();

    resetFetcher();

    IF_ASSERTS(oamEntriesCount = 0);
    IF_ASSERTS(oamEntriesNotServedCount = 0);
    IF_DEBUGGER(scanlineOamEntries.clear());

    // The first line after PPU is turn on behaves differently.
    // STAT's mode remains in HBLANK, OAM Bus is not acquired and
    // OAM Scan does not check for any sprite.
    tickSelector = &Ppu::oamScanAfterTurnOn;

    // STAT's mode goes to HBLANK, even if the PPU will start with OAM scan
    updateMode<HBLANK>();

    vram.release();
    oam.release();
}

inline bool Ppu::isLYCEqLY() const {
    // In addition to the condition (video.LYC == video.LY), LYC_EQ_LY IRQ might be disabled for several reasons.
    // Remarkably:
    // * LYC_EQ_LY is always 0 at dot 454
    // * The last scanline (LY = 153 = 0) the LY values differs from what LYC_EQ_LY reads and from the related IRQ.
    // [mooneye/lcdon_timing, daid/ppu_scanline_bgp]
    return (video.LYC == video.LY) && enableLycEqLyIrq;
}

inline void Ppu::tickStat() {
    // STAT interrupt request is checked every dot (not only during modes or lines transitions).
    // OAM Stat interrupt seems to be an exception to this rule:
    // it is raised only as a consequence of a transition of mode, not always
    // (e.g. manually writing STAT's OAM interrupt flag while in OAM does not raise a request).
    // Note that the interrupt request is done only on raising edge
    // [mooneye/stat_irq_blocking, mooneye/stat_lyc_onoff].
    const bool lycEqLy = isLYCEqLY();
    const bool lycEqLyIrq = test_bit<LYC_EQ_LY_INTERRUPT>(video.STAT) && lycEqLy;

    const uint8_t mode = keep_bits<2>(video.STAT);

    const bool hblankIrq = test_bit<HBLANK_INTERRUPT>(video.STAT) && mode == HBLANK;

    // VBlank interrupt is raised either with VBLANK or OAM STAT's flag when entering VBLANK.
    const bool vblankIrq =
        (test_bit<VBLANK_INTERRUPT>(video.STAT) || test_bit<OAM_INTERRUPT>(video.STAT)) && mode == VBLANK;

    // Eventually raise STAT interrupt.
    updateStatIrq(lycEqLyIrq || hblankIrq || vblankIrq);

    // Update STAT's LYC=LY Flag according to the current comparison
    set_bit<LYC_EQ_LY>(video.STAT, lycEqLy);
}

inline void Ppu::updateStatIrq(bool irq) {
    // Raise STAT interrupt request only on rising edge
    if (lastStatIrq < irq)
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Stat>();
    lastStatIrq = irq;
}

void Ppu::updateStatIrqForOamMode() {
    // OAM mode interrupt is not checked every dot: it is checked only here during mode transition.
    // Furthermore, it seems that having a pending LYC_EQ_LY signal high prevents the STAT IRQ to go low.
    // [daid/ppu_scanline_bgp]
    bool lycEqLyIrq = test_bit<LYC_EQ_LY_INTERRUPT>(video.STAT) && isLYCEqLY();
    updateStatIrq(test_bit<OAM_INTERRUPT>(video.STAT) || lycEqLyIrq);
}

inline void Ppu::tickWindow() {
    // Window is considered active for the rest of the frame
    // if at some point in it happens that window is enabled while LY matches WY.
    // Note that this does not mean that window will always be rendered:
    // the WX == LX condition will be checked again during pixel transfer
    // (on the other hand WY is not checked again).
    w.activeForFrame |= test_bit<WIN_ENABLE>(video.LCDC) && video.LY == video.WY;
}

// ------- PPU states ------

void Ppu::oamScanEven() {
    check(dots % 2 == 0);
    check(oamScan.count < 10);
    check(oam.isAcquiredByMe() || dots == 76 /* oam bus seems released this cycle */);

    // Read two bytes from OAM (Y and X).
    // Note that PPU cannot access OAM while DMA transfer is in progress;
    // in that case it does not read at all and the oam registers
    // will hold their old values.
    if (!oam.isAcquiredBy<Device::Dma>()) {
        registers.oam = oam.flushReadWordRequest();
    }

    tickSelector = &Ppu::oamScanOdd;

    ++dots;
}

void Ppu::oamScanOdd() {
    check(dots % 2 == 1);
    check(oamScan.count < 10);
    check(oam.isAcquiredByMe() || dots == 77 /* oam bus seems released this cycle */);

    // Read oam entry height
    const uint8_t objHeight = TILE_WIDTH << test_bit<OBJ_SIZE>(video.LCDC);

    // Check if the sprite is upon this scanline
    const uint8_t LY = video.LY;
    const uint8_t oamEntryY = registers.oam.a;
    const int32_t objY = oamEntryY - TILE_DOUBLE_HEIGHT;

    if (objY <= LY && LY < objY + objHeight) {
        const uint8_t oamEntryX = registers.oam.b;

        // Push oam entry
        if (oamEntryX < 168) {
            oamEntries[oamEntryX].emplaceBack(static_cast<uint8_t>(dots / 2),
                                              oamEntryY IF_ASSERTS_OR_DEBUGGER(COMMA oamEntryX));
            IF_DEBUGGER(scanlineOamEntries.emplaceBack(static_cast<uint8_t>(dots / 2),
                                                       oamEntryY IF_ASSERTS_OR_DEBUGGER(COMMA oamEntryX)));
            IF_ASSERTS(++oamEntriesCount);
        }

        ++oamScan.count;
    }

    ++dots;

    handleOamScanBusesOddities();

    if (dots == 80) {
        enterPixelTransfer();
    } else {
        // Check if there is room for other oam entries in this scanline
        // if that's not the case, complete oam scan now
        if (oamScan.count == 10) {
            tickSelector = &Ppu::oamScanDone;
        } else {
            // Submit a read request for the next OAM entry of the next T-cycle.
            // This is necessary to properly emulate OAM Bug on DMG, since CPU issues
            // write requests at T0 and flushes them at T1, but PPU OAM Scan Even
            // would occur at T1 only after CPU write request are flushed
            // (and that would make impossible to be aware of CPU writes).
            oam.readWordRequest(Specs::MemoryLayout::OAM::START + OAM_ENTRY_BYTES * dots / 2);
            tickSelector = &Ppu::oamScanEven;
        }
    }
}

void Ppu::oamScanDone() {
    check(oamScan.count == 10);

    ++dots;

    handleOamScanBusesOddities();

    if (dots == 80) {
        enterPixelTransfer();
    }
}

void Ppu::oamScanAfterTurnOn() {
    check(!oam.isAcquiredByMe());
    check(keep_bits<2>(video.STAT) == HBLANK);

    // First OAM Scan after PPU turn on does nothing.
    ++dots;

    if (dots == 80) {
        enterPixelTransfer();
    }
}

void Ppu::pixelTransferDummy0() {
    check(LX == 0);
    check(!w.active);
    check(oam.isAcquiredByMe());
    check(vram.isAcquiredByMe());

    if (++dots == 83) {
        // The first tile fetch is used only for make the initial SCX % 8
        // alignment possible, but this data will be thrown away in any case.
        // So filling this with junk is not a problem (even for window with WX=0).
        bgFifo.fill(DUMMY_TILE);

        // Initial SCX is not read after the beginning of Pixel Transfer.
        // It is read some T-cycles after: around here, after the first BG fetch.
        // (TODO: verify precise T-Cycle timing).
        // [mealybug/m3_scx_low_3_bits]
        pixelTransfer.initialSCX.toDiscard = mod<TILE_WIDTH>(video.SCX);

        if (pixelTransfer.initialSCX.toDiscard) {
            pixelTransfer.initialSCX.discarded = 0;

            // When SCX % 8 is > 0, window can be activated immediately before BG.
            checkWindowActivation();

            // When WX=0 and SCX=7, the pixel transfer timing seems to be
            // expected one with SCX=7, but the initial shifting applied
            // to the window is 6, not 7.
            tickSelector = w.active && pixelTransfer.initialSCX.toDiscard == 7 ? &Ppu::pixelTransferDiscard0WX0SCX7
                                                                               : &Ppu::pixelTransferDiscard0;
        } else {
            tickSelector = &Ppu::pixelTransfer0;
        }
    }
}

void Ppu::pixelTransferDiscard0() {
    check(LX == 0);
    check(pixelTransfer.initialSCX.toDiscard < 8);
    check(pixelTransfer.initialSCX.discarded < pixelTransfer.initialSCX.toDiscard);
    check(oam.isAcquiredByMe());
    check(vram.isAcquiredByMe());

    // The first SCX % 8 of a background/window tile are simply discarded.
    // Note that LX is not incremented in this case: this let the OBJ align with the BG.
    if (isBgFifoReadyToBePopped()) {
        bgFifo.popFront();

        if (++pixelTransfer.initialSCX.discarded == pixelTransfer.initialSCX.toDiscard)
            // All the SCX % 8 pixels have been discard
            tickSelector = &Ppu::pixelTransfer0;
    }

    tickFetcher();

    ++dots;
}

void Ppu::pixelTransferDiscard0WX0SCX7() {
    check(pixelTransfer.initialSCX.toDiscard == 7);
    check(w.active);

    pixelTransferDiscard0();

    if (pixelTransfer.initialSCX.discarded == 1) {
        // When WX=0 and SCX=7, the pixel transfer timing seems to be
        // expected one with SCX=7, but the initial shifting applied
        // to the window is 6, not 7, therefore we just push a dummy pixel once
        // to fix this and proceed with the standard pixelTransferDiscard0 state.
        bgFifo.pushBack({DUMMY_PIXEL});
        tickSelector = &Ppu::pixelTransferDiscard0;
    }
}

void Ppu::pixelTransfer0() {
    check(LX < 8);
    check(pixelTransfer.initialSCX.toDiscard == 0 ||
          pixelTransfer.initialSCX.discarded == pixelTransfer.initialSCX.toDiscard);
    check(oam.isAcquiredByMe());
    check(vram.isAcquiredByMe());

    bool incLX = false;

    // For LX € [0, 8) just pop the pixels but do not push them to LCD
    if (isBgFifoReadyToBePopped()) {
        bgFifo.popFront();

        if (objFifo.isNotEmpty())
            objFifo.popFront();

        incLX = true;

        if (LX + 1 == 8)
            tickSelector = &Ppu::pixelTransfer8;

        checkWindowActivation();
    }

    tickFetcher();

    if (incLX)
        increaseLX();

    ++dots;
}

void Ppu::pixelTransfer8() {
    check(LX >= 8);
    check(oam.isAcquiredByMe());
    check(vram.isAcquiredByMe());

    bool incLX = false;

    // Push a new pixel to the LCD if if the BG fifo contains
    // some pixels and it's not blocked by a sprite fetch
    if (isBgFifoReadyToBePopped()) {
        static constexpr uint8_t NO_COLOR = 4;
        uint8_t color {NO_COLOR};

        // Pop out a pixel from BG fifo
        const BgPixel bgPixel = bgFifo.popFront();

        // It seems that there is a 1 T-cycle delay between the real LCDC
        // and the LCDC the PPU sees, both for LCDC.OBJ_ENABLE and LCDC.BG_WIN_ENABLE.
        // LX == 8 seems to be an expection to this rule.
        // [mealybug/m3_lcdc_bg_en_change, mealybug/m3_lcdc_obj_en_change]
        const uint8_t lcdc = LX == 8 ? video.LCDC : lastLCDC;

        if (objFifo.isNotEmpty()) {
            const ObjPixel objPixel = objFifo.popFront();

            // Take OBJ pixel instead of the BG pixel only if all are satisfied:
            // - OBJ are still enabled
            // - OBJ pixel is opaque
            // - either BG_OVER_OBJ is disabled or the BG color is 0
            if (test_bit<OBJ_ENABLE>(lcdc) && isObjOpaque(objPixel.colorIndex) &&
                (!test_bit<BG_OVER_OBJ>(objPixel.attributes) || bgPixel.colorIndex == 0)) {
                color = resolveColor(objPixel.colorIndex,
                                     test_bit<PALETTE_NUM>(objPixel.attributes) ? video.OBP1 : video.OBP0);
            }
        }

        if (color == NO_COLOR) {
            // There is 1 T-cycle delay between video.BGP and the BGP value the PPU sees.
            // During this T-Cycle, the PPU sees the last BGP ORed with the new BGP.
            // [mealybug/m3_bgp_change]
            const uint8_t bgp = (uint8_t)video.BGP | BGP;
            color = test_bit<BG_WIN_ENABLE>(lcdc) ? resolveColor(bgPixel.colorIndex, bgp) : 0;
        }

        check(color < NUMBER_OF_COLORS);

        lcd.pushPixel(color);

        incLX = true;

        if (LX + 1 == 168) {
            increaseLX();
            ++dots;
            enterHBlank();
            return;
        }

        checkWindowActivation();
    }

    tickFetcher();

    if (incLX)
        increaseLX();

    ++dots;
}

void Ppu::hBlank() {
    check(LX == 168);
    check(video.LY < 143);

    if (++dots == 453) {
        // Eventually raise OAM interrupt
        updateStatIrqForOamMode();

        tickSelector = &Ppu::hBlank453;
    }
}

void Ppu::hBlank453() {
    check(dots == 453);

    ++dots;

    ++video.LY;

    // LYC_EQ_LY IRQ is disabled for dot 454.
    enableLycEqLyIrq = false;

    tickSelector = &Ppu::hBlank454;

    oam.acquire();
}

void Ppu::hBlank454() {
    check(dots == 454);

    ++dots;

    // Enable LYC_EQ_LY IRQ again.
    check(!enableLycEqLyIrq);
    enableLycEqLyIrq = true;

    tickSelector = &Ppu::hBlank455;
}

void Ppu::hBlank455() {
    check(dots == 455);

    dots = 0;

    enterOamScan();
}

void Ppu::hBlankLastLine() {
    check(LX == 168);
    check(video.LY == 143);
    if (++dots == 454) {
        ++video.LY;

        // LYC_EQ_LY IRQ is disabled for dot 454.
        enableLycEqLyIrq = false;

        tickSelector = &Ppu::hBlankLastLine454;
    }
}

void Ppu::hBlankLastLine454() {
    check(dots == 454);

    ++dots;

    // Enable LYC_EQ_LY IRQ again.
    check(!enableLycEqLyIrq);
    enableLycEqLyIrq = true;

    tickSelector = &Ppu::hBlankLastLine455;

    // VBlank mode is set at dot 455 (though I'm not sure about it)
    updateMode<VBLANK>();
}

void Ppu::hBlankLastLine455() {
    check(dots == 455);

    dots = 0;
    enterVBlank();
}

void Ppu::vBlank() {
    check(video.LY >= 144 && video.LY < 154);
    if (++dots == 454) {
        ++video.LY;

        // LYC_EQ_LY IRQ is disabled for dot 454.
        enableLycEqLyIrq = false;

        tickSelector = &Ppu::vBlank454;
    }
}

void Ppu::vBlank454() {
    check(dots >= 454);

    if (++dots == 456) {
        dots = 0;
        tickSelector = video.LY == 153 ? &Ppu::vBlankLastLine : &Ppu::vBlank;
    }
}

void Ppu::vBlankLastLine() {
    check(video.LY == 153);

    if (++dots == 2) {
        // LY is reset to 0
        video.LY = 0;

        // Altough LY is set to 0, LYC_EQ_LY IRQ is disabled (i.e. does not trigger for LY=0).
        enableLycEqLyIrq = false;

        tickSelector = &Ppu::vBlankLastLine2;
    }
}

void Ppu::vBlankLastLine2() {
    check(video.LY == 0);
    check(dots >= 2);

    if (++dots == 7) {
        // Enable LYC_EQ_LY IRQ again.
        enableLycEqLyIrq = true;
        tickSelector = &Ppu::vBlankLastLine7;
    }
}

void Ppu::vBlankLastLine7() {
    check(video.LY == 0);
    check(dots >= 7);

    if (++dots == 454) {
        // It seems that STAT's mode is reset the last cycle (to investigate)
        updateMode<HBLANK>();
        tickSelector = &Ppu::vBlankLastLine454;
    }
}

void Ppu::vBlankLastLine454() {
    check(video.LY == 0);
    check(dots >= 454);

    if (++dots == 456) {
        dots = 0;
        // end of vblank
        enterNewFrame();
    }
}

// ------- PPU states helpers ---------

void Ppu::enterOamScan() {
    checkCode({
        for (uint32_t i = 0; i < array_size(oamEntries); i++) {
            check(oamEntries[i].isEmpty());
        }
    });
    IF_ASSERTS(oamEntriesCount = 0);
    IF_ASSERTS(oamEntriesNotServedCount = 0);
    IF_DEBUGGER(scanlineOamEntries.clear());

    IF_DEBUGGER(timings.hBlank = 456 - (timings.oamScan + timings.pixelTransfer));

    oamScan.count = 0;

    tickSelector = &Ppu::oamScanEven;

    updateMode<OAM_SCAN>();

    check(!vram.isAcquiredByMe());
    oam.acquire();

    // Submit a read request for the first OAM entry of the next T-cycle.
    oam.readWordRequest(Specs::MemoryLayout::OAM::START);
}

void Ppu::enterPixelTransfer() {
    check(dots == 80);
    check(video.LY < 144);

    checkCode({
        check(oamScan.count <= 10);
        check(oamEntriesCount <= oamScan.count);

        uint8_t count {};
        for (uint32_t i = 0; i < array_size(oamEntries); i++) {
            count += oamEntries[i].size();
        }
        check(count == oamEntriesCount);
    });

    IF_DEBUGGER(timings.oamScan = dots);

    resetFetcher();

    tickSelector = &Ppu::pixelTransferDummy0;

    updateMode<PIXEL_TRANSFER>();

    vram.acquire();
    oam.acquire();
}

void Ppu::enterHBlank() {
    check(LX == 168);
    check(video.LY < 144);

    // clang-format off
    checkCode({
        // Pixel Transfer Timing.

        // Factors always affecting timing:
        //    REASON                                 | DOTS
        // 1) first dummy fetch                      | 3
        // 2) push first tile (that is discarded)    | 8
        // 3) push 160 pixels to LCD                 | 8 * 20

        // Factors eventually affecting timing:
        //    REASON                                 | DOTS
        // 4) initial SCX % 8 shifting               | SCX % 8
        // 5) each window triggers reset             | 6 * #(win triggers)
        //    the fetcher and takes 6 dots           |
        // 6) WX = 0 and SCX > 0 takes an extra dot  | 1 if WX=0 and SCX > 0

        // 7) from 6 to 11 for sprite fetch          | 6 to 11

        uint16_t LB = 80; // OAM Scan

        // 1) First dummy fetch
        LB += 3;

        // 2) First tile push
        LB += 8;

        // 3) 20 tiles push
        LB += 20 * 8;

        // 4) Initial SCX % 8
        LB += pixelTransfer.initialSCX.toDiscard;

        // 5) Window triggers
        LB += 6 * w.lineTriggers.size();

        // 6) WX = 0 and SCX > 0
        bool windowTriggeredAtWx0 = false;
        for (uint8_t i = 0; i < w.lineTriggers.size(); i++) {
            if (w.lineTriggers[i] == 0) {
                windowTriggeredAtWx0 = true;
                break;
            }
        }
        LB += windowTriggeredAtWx0 && pixelTransfer.initialSCX.toDiscard > 0;

        const uint8_t oamEntriesServedCount = oamEntriesCount - oamEntriesNotServedCount;

        // 7) Sprite fetch
        LB += 6 * oamEntriesServedCount /* use served count for LB */;

        uint16_t UB = LB;

        // A precise Upper Bound is difficult to predict due to sprite timing,
        // I'll let test roms verify this precisely.
        // 7) Sprite fetch
        UB += (11 - 6 /* already considered in LB */) * oamEntriesCount /* use total count for UB */;

        check(dots >= LB);
        check(dots <= UB);
    });
    // clang-format on

    IF_DEBUGGER(timings.pixelTransfer = dots - timings.oamScan);

    tickSelector = video.LY == 143 ? &Ppu::hBlankLastLine : &Ppu::hBlank;

    updateMode<HBLANK>();

    vram.release();
    oam.release();
}

void Ppu::enterVBlank() {
    IF_DEBUGGER(timings.hBlank = 456 - timings.pixelTransfer);

    tickSelector = &Ppu::vBlank;

    check(((uint8_t)video.STAT & 0b11) == VBLANK);

    interrupts.raiseInterrupt<InterruptsIO::InterruptType::VBlank>();

    check(!vram.isAcquiredByMe());
    check(!oam.isAcquiredByMe());
}

void Ppu::enterNewFrame() {
    // Reset window line counter
    // (reset to 255 so that it will go to 0 at the first window trigger).
    w.WLY = UINT8_MAX;

    // Reset window activation state
    w.activeForFrame = false;

    enterOamScan();

    // Eventually raise OAM interrupt
    updateStatIrqForOamMode();
}

inline void Ppu::tickFetcher() {
    (this->*(fetcherTickSelector))();
}

void Ppu::resetFetcher() {
    LX = 0;
    bgFifo.clear();
    objFifo.clear();
    isFetchingSprite = false;
    w.active = false;
    w.justActivated = false;
    IF_ASSERTS_OR_DEBUGGER(w.lineTriggers.clear());
    bwf.LX = 0;
    bwf.interruptedFetch.hasData = false;
    wf.tilemapX = 0;
    fetcherTickSelector = &Ppu::bgwinPrefetcherGetTile0;
}

template <uint8_t Mode>
inline void Ppu::updateMode() {
    check(Mode <= 0b11);
    video.STAT = ((uint8_t)video.STAT & 0b11111100) | Mode;
}

inline void Ppu::increaseLX() {
    check(oamEntriesCount >= oamEntries[LX].size());
    IF_ASSERTS(oamEntriesNotServedCount += oamEntries[LX].size()); // update with the real served oam entries count.
    check(oamEntriesNotServedCount <= oamEntriesCount);

    // Clear oam entries of current LX (it might contain OBJs not served).
    oamEntries[LX].clear();

    // Increase LX.
    ++LX;
}

inline bool Ppu::isBgFifoReadyToBePopped() const {
    // The condition for which the bg fifo can be popped are:
    // - the bg fifo is not empty
    // - the fetcher is not fetching a sprite
    // - there are no more oam entries for this LX to be fetched or obj are disabled in LCDC
    return bgFifo.isNotEmpty() && !isFetchingSprite && (oamEntries[LX].isEmpty() || !test_bit<OBJ_ENABLE>(video.LCDC));
}

inline bool Ppu::isObjReadyToBeFetched() const {
    // The condition for which a obj fetch can be served are:
    // - there is at least a pending oam entry for the current LX
    // - obj are enabled in LCDC
    return oamEntries[LX].isNotEmpty() && test_bit<OBJ_ENABLE>(video.LCDC);
}

inline void Ppu::checkWindowActivation() {
    // The condition for which the window can be triggered are:
    // - at some point in the frame LY was equal to WY
    // - window should is enabled LCDC
    // - pixel transfer LX matches WX
    //     furthermore, it seems that if LCDC.WIN_ENABLE was off and
    //     is turned on while LX == WX + 1, window is activated
    //     (even if it would be late by 1-cycle to be activated).
    //     [mealybug/m3_lcdc_win_en_change_multiple_wx]
    if (w.activeForFrame && !w.active && test_bit<WIN_ENABLE>(video.LCDC) &&
        (LX == WX /* standard case */ ||
         (LX == WX + 1 && !test_bit<WIN_ENABLE>(lastLCDC) /* window just enabled case */))) {
        setupFetcherForWindow();
    }
}

inline void Ppu::setupFetcherForWindow() {
    check(!w.active);

    // Activate window.
    w.active = true;
    w.justActivated = true;

    // Increase the window line counter.
    ++w.WLY;

    IF_ASSERTS_OR_DEBUGGER(w.lineTriggers.pushBack(WX));

    // Reset the window tile counter.
    wf.tilemapX = 0;

    // Throw away the pixels in the BG fifo.
    bgFifo.clear();

    // Setup the fetcher to fetch a window tile.
    fetcherTickSelector = &Ppu::winPrefetcherActivating;
}

inline void Ppu::handleOamScanBusesOddities() {
    check(keep_bits<2>(video.STAT) == OAM_SCAN);

    if (dots == 76) {
        // OAM bus seems to be released (i.e. writes to OAM works normally) just for this cycle
        // [mooneye/lcdon_write_timing]
        oam.release();
    } else if (dots == 78) {
        // OAM bus is re-acquired this cycle
        // [mooneye/lcdon_write_timing]
        oam.acquire();

        // VRAM bus is acquired one cycle before STAT is updated
        // [mooneye/lcdon_timing]
        vram.acquire();
    }
}

// -------- Fetcher states ---------

// ----------- bg / win ------------

void Ppu::bgwinPrefetcherGetTile0() {
    check(!isFetchingSprite);

    // Check whether fetch the tile for the window or for the BG
    if (w.active) {
        if (test_bit<WIN_ENABLE>(video.LCDC) /* TODO: other conditions checked? WY?*/) {
            // Proceed to fetch window
            winPrefetcherGetTile0();
            return;
        }

        // Abort window fetch and proceed with BG fetch
        w.active = false;
    }

    bgPrefetcherGetTile0();
}

void Ppu::bgPrefetcherGetTile0() {
    check(!isFetchingSprite);

    // The prefetcher computes at this phase only the tile base address.
    // based on the tilemap X and Y coordinate and the tile map to use.
    // [mealybug/m3_lcdc_bg_map_change].
    // The real tile address that depends on the tile Y can be affected by
    // successive SCY write that might happen during the fetcher's GetTile phases.
    // [mealybug/m3_scy_change].
    setupBgPixelSliceFetcherTilemapTileAddress();

    fetcherTickSelector = &Ppu::bgPrefetcherGetTile1;
}

void Ppu::bgPrefetcherGetTile1() {
    check(!isFetchingSprite);
    fetcherTickSelector = &Ppu::bgPixelSliceFetcherGetTileDataLow0;
}

void Ppu::bgPixelSliceFetcherGetTileDataLow0() {
    check(!isFetchingSprite);

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore changes to SCY or BG_WIN_TILE_DATA during these phases may
    // have bitplane desync effects
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_scy_change, mealybug/m3_lcdc_tile_sel_change]
    setupBgPixelSliceFetcherTileDataAddress();

    psf.tileDataLow = vram.read(psf.vTileDataAddress);

    fetcherTickSelector = &Ppu::bgPixelSliceFetcherGetTileDataLow1;
}

void Ppu::bgPixelSliceFetcherGetTileDataLow1() {
    check(!isFetchingSprite);

    fetcherTickSelector = &Ppu::bgPixelSliceFetcherGetTileDataHigh0;
}

void Ppu::bgPixelSliceFetcherGetTileDataHigh0() {
    check(!isFetchingSprite);

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTiledataHigh just before access it from VRAM.
    // Therefore changes to SCY or BG_WIN_TILE_DATA during these phases may
    // have bitplane desync effects
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_scy_change, mealybug/m3_lcdc_tile_sel_change]
    setupBgPixelSliceFetcherTileDataAddress();

    psf.tileDataHigh = vram.read(psf.vTileDataAddress + 1);

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1;
}

// Some random notes about window //

/*
 *  Pixel Transfer Window Timing.
 *
 *   WX | SCX |   LX=1   |   LX=8      | Actual SCX discard
 *   -------------------------------------------------------
 *          (no window case)
 *  inf |  0  |   X      |  Y = X + 7  | 0
 *
 *       (window WX=0 window case)
 *    0 |  0  |   X      |  Y + 6      | 0
 *    0 |  1  |   X + 8  |  Y + 8      | 1
 *    0 |  2  |   X + 9  |  Y + 9      | 2
 *    0 |  3  |   X + 10 |  Y + 10     | 3
 *    0 |  4  |   X + 11 |  Y + 11     | 4
 *    0 |  5  |   X + 12 |  Y + 12     | 5
 *    0 |  6  |   X + 13 |  Y + 13     | 6
 *    0 |  7  |   X + 14 |  Y + 14     | 6 (not 7)
 *
 *    Probably X = 84, Y = 91.
 */

/*
 *  Window Abort BG Reprise.
 *
 *  BG Phase           |  BG tilemap X
 *  ----------------------------------
 *  GetTile 0          | +0
 *  GetTile 1          | +0
 *  GetTileDataLow 0   | +0
 *  GetTileDataLow 1   | +0
 *  GetTileDataHigh 0  | +0
 *  GetTileDataHigh 1  | +0
 *  Push               | +1
 */

/*
 *  Window 00 Pixel Glitch.
 *
 *  Given WX1, WX2:
 *
 *  Glitch Same Line   = 1 if (WX2 = WX1 + 8n)        ∃n € N
 *  Glitch Next Lines  = 1 if (WX2 = 7 - SCX + 8n)    ∃n € N
 */

void Ppu::winPrefetcherActivating() {
    check(!isFetchingSprite);
    check(w.activeForFrame);
    check(w.active);
    check(w.justActivated);
    check(test_bit<WIN_ENABLE>(video.LCDC));

    // It seems that the t-cycle the window is activated is just wasted and the
    // fetcher does not advance (this is deduced by the fact that the rom's expected
    // behavior of the t-cycles of this first window fetch matches the normal
    // window states only if such nop t-cycle is inserted after the window activation).
    // Note that the timing added by the window fetch is still 6 dots: therefore
    // is likely that the first window tile is pushed with the GetTile1 fetcher
    // phase and does not waste an additional t-cycle in Push.
    // [mealybug/m3_lcdc_win_map_change, mealybug/m3_lcdc_win_en_change_multiple_wx],
    fetcherTickSelector = &Ppu::winPrefetcherGetTile0;
}

void Ppu::winPrefetcherGetTile0() {
    check(!isFetchingSprite);
    check(w.activeForFrame);
    check(w.active);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!test_bit<WIN_ENABLE>(video.LCDC)) {
        bgPrefetcherGetTile0();
    } else {
        // The prefetcher computes at this phase only the tile base address.
        // based on the tilemap X and Y coordinate and the tile map to use.
        // [mealybug/m3_lcdc_win_map_change].
        setupWinPixelSliceFetcherTilemapTileAddress();

        fetcherTickSelector = &Ppu::winPrefetcherGetTile1;
    }

    if (w.justActivated) {
        // The window activation shifts back the BG prefetcher by one tile.
        // Note that this happens here (not when the window is triggered),
        // therefore there is a 1 t-cycle window opportunity to show
        // the same tile twice if window is disabled just after it is activated.
        bwf.LX -= TILE_WIDTH;
    }
}

void Ppu::winPrefetcherGetTile1() {
    check(!isFetchingSprite);
    check(w.activeForFrame);
    check(w.active);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!test_bit<WIN_ENABLE>(video.LCDC)) {
        bgPrefetcherGetTile1();
        return;
    }

    fetcherTickSelector = &Ppu::winPixelSliceFetcherGetTileDataLow0;
}

void Ppu::winPixelSliceFetcherGetTileDataLow0() {
    check(!isFetchingSprite);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!test_bit<WIN_ENABLE>(video.LCDC)) {
        bgPixelSliceFetcherGetTileDataLow0();
        return;
    }

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTiledataHigh just before access it from VRAM.
    // Therefore changes to (WY or?) BG_WIN_TILE_DATA during these phases may
    // have bitplane desync effects
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_lcdc_tile_sel_win_change]
    setupWinPixelSliceFetcherTileDataAddress();

    psf.tileDataLow = vram.read(psf.vTileDataAddress);

    fetcherTickSelector = &Ppu::winPixelSliceFetcherGetTileDataLow1;
}

void Ppu::winPixelSliceFetcherGetTileDataLow1() {
    check(!isFetchingSprite);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!test_bit<WIN_ENABLE>(video.LCDC)) {
        bgPixelSliceFetcherGetTileDataLow1();
        return;
    }

    fetcherTickSelector = &Ppu::winPixelSliceFetcherGetTileDataHigh0;
}

void Ppu::winPixelSliceFetcherGetTileDataHigh0() {
    check(!isFetchingSprite);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!test_bit<WIN_ENABLE>(video.LCDC)) {
        bgPixelSliceFetcherGetTileDataHigh0();
        return;
    }

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTiledataHigh just before access it from VRAM.
    // Therefore changes to (WY or?) BG_WIN_TILE_DATA during these phases may
    // have bitplane desync effects
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_lcdc_tile_sel_win_change]
    setupWinPixelSliceFetcherTileDataAddress();

    psf.tileDataHigh = vram.read(psf.vTileDataAddress + 1);

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1;
}

void Ppu::bgwinPixelSliceFetcherGetTileDataHigh1() {
    check(!isFetchingSprite);

    // The bg/win fetcher tile is increased only at this step, not before or after.
    // Therefore, if the window aborts a bg fetch before this step,
    // the bg tile does not increment: it increments only if the
    // fetcher reaches the push stage (or a sprite aborts it)
    bwf.LX += TILE_WIDTH; // automatically handle mod 8 by overflowing

    // If there is a pending obj hit (and the bg fifo is not empty),
    // discard the fetched bg pixels and restart the obj prefetcher with the obj hit
    // note: the first obj prefetcher tick should overlap this tick, not the push one
    if (isObjReadyToBeFetched() && bgFifo.isNotEmpty()) {
        // The bg/win tile fetched is not thrown away: instead it is
        // cached so that the fetcher can start with it after the sprite
        // has been merged into obj fifo
        cacheBgWinFetch();

        isFetchingSprite = true;
        of.entry = oamEntries[LX].pullBack();
        objPrefetcherGetTile0();
        return;
    }

    if (w.justActivated) {
        // The fetcher immediately pushes the window tile if the window
        // has just been activated (i.e. this is the first window fetch).
        check(bgFifo.isEmpty());
        bgwinPixelSliceFetcherPush();
        w.justActivated = false;
    } else {
        // Standard case.
        fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherPush;
    }
}

void Ppu::bgwinPixelSliceFetcherPush() {
    check(!isFetchingSprite);

    /*
     * As far as I'm understanding, there are 4 possible cases:
     *
     * [canPushToBgFifo] | [isObjReadyToBeFetched]
     * ---------------------------------------------------------
     * bgFifo.IsEmpty()  | oamEntries[LX].isNotEmpty() &&
     *                   | test_bit<OBJ_ENABLE>(video.LCDC)
     * ---------------------------------------------------------
     *        0         |           0              | wait (nop)
     *        0         |           1              | cache bg fetch and start obj prefetcher (tick now)
     *        1         |           0              | push to bg fifo and prepare for bg/win prefetcher (tick next dot)
     *        1         |           1              | push to bg fifo and start obj prefetcher (tick now)
     */

    // The pixels can be pushed only if the bg fifo is empty,
    // otherwise wait in push state until bg fifo is emptied
    const bool canPushToBgFifo = bgFifo.isEmpty();
    if (canPushToBgFifo) {
        // If window activation conditions were met in the frame, then a
        // glitch can occur: if this push stage happens when LX == WX,
        // than a 00 pixel is pushed into the bg fifo instead of the fetched tile.
        // Therefore, the push of the tile that was about to happen is postponed by 1 dot
        if (w.activeForFrame && LX == WX && LX > 8 /* TODO: not sure about this */) {
            bgFifo.pushBack(BgPixel {0});
            return;
        }

        // Push pixels into bg fifo
        bgFifo.fill(&TILE_ROW_DATA_TO_ROW_PIXELS[psf.tileDataHigh << 8 | psf.tileDataLow]);

        fetcherTickSelector = &Ppu::bgwinPrefetcherGetTile0;

        // Sprite fetches are ignored just after a window tile push.
        if (w.active)
            return;
    }

    // If there is a pending obj hit, discard the fetched bg pixels
    // and restart the obj prefetcher with the obj hit.
    // Note: the first obj prefetcher tick overlap this tick.
    if (isObjReadyToBeFetched()) {
        if (!canPushToBgFifo) {
            // The bg/win tile fetched is not thrown away: instead it is
            // cached so that the fetcher can start with it after the sprite
            // has been merged into obj fifo.
            cacheBgWinFetch();
        }

        isFetchingSprite = true;
        of.entry = oamEntries[LX].pullBack();
        objPrefetcherGetTile0();
    }
}

// ------------- obj ---------------

void Ppu::objPrefetcherGetTile0() {
    check(isFetchingSprite);

    // Read two bytes from OAM (Tile Number and Attributes).
    // Note that if DMA transfer is in progress conflicts can occur and
    // we might end up reading from the OAM address the DMA is writing to
    // (but only if DMA writing request is pending, i.e. it is in T0 or T1).
    // [hacktix/strikethrough]

    // TODO: figure out exact timing of read request and read flush for obj prefetcher.
    oam.readWordRequest(Specs::MemoryLayout::OAM::START + OAM_ENTRY_BYTES * of.entry.number +
                        Specs::Bytes::OAM::TILE_NUMBER);

    fetcherTickSelector = &Ppu::objPrefetcherGetTile1;
}

void Ppu::objPrefetcherGetTile1() {
    check(isFetchingSprite);

    // TODO: figure out exact timing of read request and read flush for obj prefetcher.
    registers.oam = oam.flushReadWordRequest();

    of.tileNumber = registers.oam.a;
    of.attributes = registers.oam.b;

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataLow0;
}

void Ppu::objPixelSliceFetcherGetTileDataLow0() {
    check(isFetchingSprite);
    check(bgFifo.isNotEmpty());

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataLow1;
}

void Ppu::objPixelSliceFetcherGetTileDataLow1() {
    check(isFetchingSprite);
    check(bgFifo.isNotEmpty());

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore changes to OBJ_SIZE during these phases may have bitplane
    // desync effects.
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_lcdc_obj_size_change, mealybug/m3_lcdc_obj_size_change_scx]
    setupObjPixelSliceFetcherTileDataAddress();

    psf.tileDataLow = vram.read(psf.vTileDataAddress);

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataHigh0;
}

void Ppu::objPixelSliceFetcherGetTileDataHigh0() {
    check(isFetchingSprite);
    check(bgFifo.isNotEmpty());

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo;
}

void Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo() {
    check(isFetchingSprite);
    check(of.entry.x == LX);
    check(bgFifo.isNotEmpty());

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore changes to OBJ_SIZE during these phases may have bitplane
    // desync effects.
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_lcdc_obj_size_change, mealybug/m3_lcdc_obj_size_change_scx]
    setupObjPixelSliceFetcherTileDataAddress();

    psf.tileDataHigh = vram.read(psf.vTileDataAddress + 1);

    PixelColorIndex objPixelsColors[8];
    const uint8_t(*pixelsMapPtr)[8] =
        test_bit<X_FLIP>(of.attributes) ? TILE_ROW_DATA_TO_ROW_PIXELS_FLIPPED : TILE_ROW_DATA_TO_ROW_PIXELS;
    memcpy(objPixelsColors, &pixelsMapPtr[psf.tileDataHigh << 8 | psf.tileDataLow], 8);

    ObjPixel objPixels[8];
    for (uint8_t i = 0; i < 8; i++) {
        objPixels[i] = {objPixelsColors[i], of.attributes, of.entry.number, LX};
    }

    // Handle sprite-to-sprite conflicts by merging the new obj pixels with
    // the pixels already in the obj fifo
    // "The smaller the X coordinate, the higher the priority."
    // "When X coordinates are identical, the object located first in OAM has higher priority."

    const uint8_t objFifoSize = objFifo.size();
    uint8_t i = 0;

    // Handle conflict between the new and the old obj pixels
    while (i < objFifoSize) {
        const ObjPixel& fifoObjPixel = objFifo[i];
        const ObjPixel& objPixel = objPixels[i];

        check(objPixel.x >= fifoObjPixel.x);

        // Overwrite pixel in fifo with pixel in sprite if:
        // 1. Pixel in sprite is opaque and pixel in fifo is transparent
        // 2. Both pixels in sprite and fifo are opaque but pixel in sprite
        //    has either lower x or, if x is equal, lowest oam number.
        if ((isObjOpaque(objPixel.colorIndex) && isObjTransparent(fifoObjPixel.colorIndex)) ||
            ((isObjOpaque(objPixel.colorIndex) && isObjOpaque(fifoObjPixel.colorIndex)) &&
             (objPixel.x == fifoObjPixel.x && objPixel.number < fifoObjPixel.number))) {
            objFifo[i] = objPixel;
        }

        ++i;
    }

    // Push the remaining obj pixels
    while (i < 8) {
        objFifo.pushBack(objPixels[i++]);
    }

    check(objFifo.isFull());

    if (isObjReadyToBeFetched()) {
        // Still oam entries hit to be served for this x: setup the fetcher
        // for another obj fetch
        of.entry = oamEntries[LX].pullBack();
        fetcherTickSelector = &Ppu::objPrefetcherGetTile0;
    } else {
        // No more oam entries to serve for this x: setup to fetcher with
        // the cached tile data that has been interrupted by this obj fetch
        isFetchingSprite = false;

        if (bwf.interruptedFetch.hasData) {
            restoreBgWinFetch();
            fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherPush;
        } else {
            fetcherTickSelector = &Ppu::bgwinPrefetcherGetTile0;
        }
    }
}

// ------- Fetcher states helpers ---------

inline void Ppu::setupObjPixelSliceFetcherTileDataAddress() {
    const bool isDoubleHeight = test_bit<OBJ_SIZE>(video.LCDC);
    const uint8_t heightMask = isDoubleHeight ? 0xF : 0x7;

    const uint8_t objY = of.entry.y - TILE_DOUBLE_HEIGHT;

    // The OBJ tileY is always mapped within the range [0:objHeight).
    uint8_t tileY = (video.LY - objY) & heightMask;

    if (test_bit<Y_FLIP>(of.attributes))
        // Take the opposite row within objHeight.
        tileY ^= heightMask;

    // Last bit is ignored for 8x16 OBJs.
    const uint8_t tileNumber = (isDoubleHeight ? (of.tileNumber & 0xFE) : of.tileNumber);

    const uint16_t vTileAddr = TILE_BYTES * tileNumber;

    psf.vTileDataAddress = vTileAddr + TILE_ROW_BYTES * tileY;
}

inline void Ppu::setupBgPixelSliceFetcherTilemapTileAddress() {
    const uint8_t tilemapX = mod<TILEMAP_WIDTH>((bwf.LX + video.SCX) / TILE_WIDTH);
    const uint8_t tilemapY = mod<TILEMAP_HEIGHT>((video.LY + video.SCY) / TILE_HEIGHT);

    const uint16_t vTilemapAddr = test_bit<BG_TILE_MAP>(video.LCDC) ? 0x1C00 : 0x1800; // 0x9800 or 0x9C00 (global)
    bwf.vTilemapTileAddr = vTilemapAddr + (TILEMAP_WIDTH * TILEMAP_CELL_BYTES * tilemapY) + tilemapX;

    IF_DEBUGGER(bwf.tilemapX = tilemapX);
    IF_DEBUGGER(bwf.tilemapY = tilemapY);
    IF_DEBUGGER(bwf.vTilemapAddr = vTilemapAddr);
}

inline void Ppu::setupBgPixelSliceFetcherTileDataAddress() {
    const uint8_t tileNumber = vram.read(bwf.vTilemapTileAddr);

    const uint16_t vTileAddr = test_bit<BG_WIN_TILE_DATA>(video.LCDC)
                                   ?
                                   // unsigned addressing mode with 0x8000 as (global) base address
                                   0x0000 + TILE_BYTES * tileNumber
                                   :
                                   // signed addressing mode with 0x9000 as (global) base address
                                   0x1000 + TILE_BYTES * to_signed(tileNumber);
    const uint8_t tileY = mod<TILE_HEIGHT>(video.LY + video.SCY);

    psf.vTileDataAddress = vTileAddr + TILE_ROW_BYTES * tileY;
}

inline void Ppu::setupWinPixelSliceFetcherTilemapTileAddress() {
    // The window prefetcher has its own internal counter to determine the tile to fetch
    const uint8_t tilemapX = wf.tilemapX++;
    const uint8_t tilemapY = w.WLY / TILE_HEIGHT;
    const uint16_t vTilemapAddr = test_bit<WIN_TILE_MAP>(video.LCDC) ? 0x1C00 : 0x1800; // 0x9800 or 0x9C00 (global)
    bwf.vTilemapTileAddr = vTilemapAddr + (TILEMAP_WIDTH * TILEMAP_CELL_BYTES * tilemapY) + tilemapX;

    IF_DEBUGGER(bwf.tilemapX = tilemapX);
    IF_DEBUGGER(bwf.tilemapY = tilemapY);
    IF_DEBUGGER(bwf.vTilemapAddr = vTilemapAddr);
}

inline void Ppu::setupWinPixelSliceFetcherTileDataAddress() {
    const uint8_t tileNumber = vram.read(bwf.vTilemapTileAddr);

    const uint16_t vTileAddr = test_bit<BG_WIN_TILE_DATA>(video.LCDC)
                                   ?
                                   // unsigned addressing mode with 0x8000 as (global) base address
                                   0x0000 + TILE_BYTES * tileNumber
                                   :
                                   // signed addressing mode with 0x9000 as (global) base address
                                   0x1000 + TILE_BYTES * to_signed(tileNumber);
    const uint8_t tileY = mod<TILE_HEIGHT>(w.WLY);

    psf.vTileDataAddress = vTileAddr + TILE_ROW_BYTES * tileY;
}

inline void Ppu::cacheBgWinFetch() {
    bwf.interruptedFetch.tileDataLow = psf.tileDataLow;
    bwf.interruptedFetch.tileDataHigh = psf.tileDataHigh;
    check(!bwf.interruptedFetch.hasData);
    bwf.interruptedFetch.hasData = true;
}

inline void Ppu::restoreBgWinFetch() {
    psf.tileDataLow = bwf.interruptedFetch.tileDataLow;
    psf.tileDataHigh = bwf.interruptedFetch.tileDataHigh;
    check(bwf.interruptedFetch.hasData);
    bwf.interruptedFetch.hasData = false;
}

void Ppu::saveState(Parcel& parcel) const {
    {
        uint8_t i = 0;

        while (i < (uint8_t)array_size(TICK_SELECTORS) && tickSelector != TICK_SELECTORS[i])
            ++i;
        check(i < array_size(TICK_SELECTORS));
        parcel.writeUInt8(i);

        i = 0;
        while (i < (uint8_t)array_size(FETCHER_TICK_SELECTORS) && fetcherTickSelector != FETCHER_TICK_SELECTORS[i])
            ++i;
        check(i < array_size(FETCHER_TICK_SELECTORS));
        parcel.writeUInt8(i);
    }

    parcel.writeBool(on);
    parcel.writeBool(lastStatIrq);
    parcel.writeBool(enableLycEqLyIrq);
    parcel.writeUInt16(dots);
    parcel.writeUInt8(LX);
    parcel.writeUInt8(BGP);
    parcel.writeUInt8(WX);

    parcel.writeBytes(bgFifo.data, sizeof(bgFifo.data));
    parcel.writeUInt8(bgFifo.cursor);

    parcel.writeBytes(objFifo.data, sizeof(objFifo.data));
    parcel.writeUInt8(objFifo.start);
    parcel.writeUInt8(objFifo.end);
    parcel.writeUInt8(objFifo.count);

    parcel.writeBytes(oamEntries, sizeof(oamEntries));
    IF_ASSERTS(parcel.writeUInt8(oamEntriesCount));
    IF_ASSERTS(parcel.writeUInt8(oamEntriesNotServedCount));

    parcel.writeBool(isFetchingSprite);

    parcel.writeUInt8(registers.oam.a);
    parcel.writeUInt8(registers.oam.b);

    parcel.writeUInt8(oamScan.count);

    parcel.writeUInt8(pixelTransfer.initialSCX.toDiscard);
    parcel.writeUInt8(pixelTransfer.initialSCX.discarded);

    parcel.writeUInt8(w.WLY);
    parcel.writeBool(w.active);
    parcel.writeBool(w.justActivated);

#ifdef ASSERTS_OR_DEBUGGER_ENABLED
    parcel.writeUInt8(w.lineTriggers.size());
    for (uint8_t i = 0; i < w.lineTriggers.size(); i++)
        parcel.writeUInt8(w.lineTriggers[i]);
#endif

    parcel.writeUInt8(bwf.LX);
    parcel.writeUInt8(bwf.vTilemapTileAddr);
    IF_DEBUGGER(parcel.writeUInt8(bwf.tilemapX));
    IF_DEBUGGER(parcel.writeUInt8(bwf.tilemapY));
    IF_DEBUGGER(parcel.writeUInt8(bwf.vTilemapAddr));
    parcel.writeBool(bwf.interruptedFetch.hasData);
    parcel.writeUInt8(bwf.interruptedFetch.tileDataLow);
    parcel.writeUInt8(bwf.interruptedFetch.tileDataHigh);

    parcel.writeUInt8(wf.tilemapX);

    parcel.writeUInt8(of.entry.number);
    parcel.writeUInt8(of.entry.y);
    IF_ASSERTS(parcel.writeUInt8(of.entry.x));
    parcel.writeUInt8(of.tileNumber);
    parcel.writeUInt8(of.attributes);

    parcel.writeUInt16(psf.vTileDataAddress);
    parcel.writeUInt8(psf.tileDataLow);
    parcel.writeUInt8(psf.tileDataHigh);

    IF_DEBUGGER(parcel.writeUInt64(cycles));
}

void Ppu::loadState(Parcel& parcel) {
    const uint8_t tickSelectorNumber = parcel.readUInt8();
    check(tickSelectorNumber < array_size(TICK_SELECTORS));
    tickSelector = TICK_SELECTORS[tickSelectorNumber];

    const uint8_t fetcherTickSelectorNumber = parcel.readUInt8();
    check(fetcherTickSelectorNumber < array_size(FETCHER_TICK_SELECTORS));
    fetcherTickSelector = FETCHER_TICK_SELECTORS[fetcherTickSelectorNumber];

    on = parcel.readBool();
    lastStatIrq = parcel.readBool();
    enableLycEqLyIrq = parcel.readBool();
    dots = parcel.readUInt16();
    LX = parcel.readUInt8();
    BGP = parcel.readUInt8();
    WX = parcel.readUInt8();

    parcel.readBytes(bgFifo.data, sizeof(bgFifo.data));
    bgFifo.cursor = parcel.readUInt8();

    parcel.readBytes(objFifo.data, sizeof(objFifo.data));
    objFifo.start = parcel.readUInt8();
    objFifo.end = parcel.readUInt8();
    objFifo.count = parcel.readUInt8();

    parcel.readBytes(oamEntries, sizeof(oamEntries));
    IF_ASSERTS(oamEntriesCount = parcel.readUInt8());
    IF_ASSERTS(oamEntriesNotServedCount = parcel.readUInt8());

    isFetchingSprite = parcel.readBool();

    registers.oam.a = parcel.readUInt8();
    registers.oam.b = parcel.readUInt8();

    oamScan.count = parcel.readUInt8();

    pixelTransfer.initialSCX.toDiscard = parcel.readUInt8();
    pixelTransfer.initialSCX.discarded = parcel.readUInt8();

    w.WLY = parcel.readUInt8();
    w.active = parcel.readBool();
    w.justActivated = parcel.readBool();

#ifdef ASSERTS_OR_DEBUGGER_ENABLED
    uint8_t lineTriggers = parcel.readUInt8();
    for (uint8_t i = 0; i < lineTriggers; i++)
        w.lineTriggers.pushBack(parcel.readUInt8());
#endif

    bwf.LX = parcel.readUInt8();
    bwf.vTilemapTileAddr = parcel.readUInt8();
    IF_DEBUGGER(bwf.tilemapX = parcel.readUInt8());
    IF_DEBUGGER(bwf.tilemapY = parcel.readUInt8());
    IF_DEBUGGER(bwf.vTilemapAddr = parcel.readUInt8());
    bwf.interruptedFetch.hasData = parcel.readBool();
    bwf.interruptedFetch.tileDataLow = parcel.readUInt8();
    bwf.interruptedFetch.tileDataHigh = parcel.readUInt8();

    wf.tilemapX = parcel.readUInt8();

    of.entry.number = parcel.readUInt8();
    of.entry.y = parcel.readUInt8();
    IF_ASSERTS(of.entry.x = parcel.readUInt8());
    of.tileNumber = parcel.readUInt8();
    of.attributes = parcel.readUInt8();

    psf.vTileDataAddress = parcel.readUInt16();
    psf.tileDataLow = parcel.readUInt8();
    psf.tileDataHigh = parcel.readUInt8();

    IF_DEBUGGER(cycles = parcel.readUInt64());
}

void Ppu::reset() {
    tickSelector = IF_BOOTROM_ELSE(&Ppu::oamScanEven, &Ppu::vBlankLastLine7);
    fetcherTickSelector = &Ppu::bgPrefetcherGetTile0;

    on = true;

    lastStatIrq = false;
    enableLycEqLyIrq = true;

    dots = IF_BOOTROM_ELSE(0, 395);
    LX = 0;

    BGP = IF_BOOTROM_ELSE(0, 0xFC);
    WX = 0;
    lastLCDC = IF_BOOTROM_ELSE(0x80, 0x85);

    bgFifo.clear();
    objFifo.clear();

    for (uint32_t i = 0; i < array_size(oamEntries); i++)
        oamEntries[i].clear();

    IF_ASSERTS(oamEntriesCount = 0);
    IF_ASSERTS(oamEntriesNotServedCount = 0);

    IF_DEBUGGER(scanlineOamEntries.clear());

    isFetchingSprite = false;

    IF_DEBUGGER(timings.oamScan = 0);
    IF_DEBUGGER(timings.pixelTransfer = 0);
    IF_DEBUGGER(timings.hBlank = 0);

    registers.oam.a = 0;
    registers.oam.a = 0;

    oamScan.count = 0;

    pixelTransfer.initialSCX.toDiscard = 0;
    pixelTransfer.initialSCX.discarded = 0;

    w.activeForFrame = false;
    w.WLY = UINT8_MAX;
    w.active = false;
    w.justActivated = false;
    IF_ASSERTS_OR_DEBUGGER(w.lineTriggers.clear());

    bwf.LX = 0;
    bwf.vTilemapTileAddr = 0;

    IF_DEBUGGER(bwf.tilemapX = 0);
    IF_DEBUGGER(bwf.tilemapY = 0);
    IF_DEBUGGER(bwf.vTilemapAddr = 0);

    bwf.interruptedFetch.hasData = false;
    bwf.interruptedFetch.tileDataLow = 0;
    bwf.interruptedFetch.tileDataHigh = 0;
    wf.tilemapX = 0;

    of.entry.number = 0;
    of.entry.y = 0;
    IF_ASSERTS_OR_DEBUGGER(of.entry.x = 0);
    of.tileNumber = 0;
    of.attributes = 0;

    psf.vTileDataAddress = 0;
    psf.tileDataLow = 0;
    psf.tileDataHigh = 0;

    IF_DEBUGGER(cycles = 0);

    IF_NOT_BOOTROM(oam.acquire());
}
