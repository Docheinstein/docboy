#include "ppu.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/lcd/lcd.h"
#include "docboy/memory/memory.hpp"
#include "docboy/ppu/video.h"
#include "pixelmap.h"
#include "utils/casts.hpp"
#include "utils/macros.h"
#include "utils/math.h"
#include "utils/traits.h"

using namespace Specs::Bits::Video::LCDC;
using namespace Specs::Bits::Video::STAT;
using namespace Specs::Bits::OAM::Attributes;

using namespace Specs::Ppu::Modes;

static constexpr uint8_t TILE_WIDTH = 8;
static constexpr uint8_t TILE_HEIGHT = 8;
static constexpr uint8_t TILE_DOUBLE_HEIGHT = 16;
static constexpr uint8_t TILE_BYTES = 16;
static constexpr uint8_t TILE_ROW_BYTES = 2;

static constexpr uint8_t TILEMAP_WIDTH = 32;
static constexpr uint8_t TILEMAP_HEIGHT = 32;
static constexpr uint8_t TILEMAP_CELL_BYTES = 1;

static constexpr uint8_t OAM_ENTRY_BYTES = 4;

static constexpr uint8_t OBJ_COLOR_INDEX_TRANSPARENT = 0;

static constexpr uint8_t NUMBER_OF_COLORS = 4;
static constexpr uint8_t BITS_PER_PIXEL = 2;

static inline bool isObjOpaque(const uint8_t colorIndex) {
    return colorIndex != OBJ_COLOR_INDEX_TRANSPARENT;
}

static inline bool isObjTransparent(const uint8_t colorIndex) {
    return colorIndex == OBJ_COLOR_INDEX_TRANSPARENT;
}

static inline uint8_t resolveColor(const uint8_t colorIndex, const uint8_t palette) {
    check(colorIndex < NUMBER_OF_COLORS);
    return keep_bits<BITS_PER_PIXEL>(palette >> (BITS_PER_PIXEL * colorIndex));
}

static constexpr uint8_t DUMMY_TILE[8] {};

Ppu::Ppu(Lcd& lcd, VideoIO& video, InterruptsIO& interrupts, VramBus::View<Device::Ppu> vramBus,
         OamBus::View<Device::Ppu> oamBus) :
    lcd(lcd),
    video(video),
    interrupts(interrupts),
    vram(vramBus),
    oam(oamBus) {
    IF_NOT_BOOTROM(oamBus.acquire());
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
    lastBGP = video.BGP;

    // Update STAT's LYC_EQ_LY and eventually raise STAT interrupt
    tickStat();

    check(dots < 456);

    IF_DEBUGGER(++cycles);
}

inline void Ppu::tickStat() {
    // STAT interrupt request is checked every dot (not only during modes or lines transitions).
    // OAM Stat interrupt seems to be an exception to this rule:
    // it is raised only as a consequence of a transition of mode, not always
    // (e.g. manually writing STAT's OAM interrupt flag while in OAM does not raise a request).
    // Note that the interrupt request is done only on raising edge
    // [mooneye's stat_irq_blocking and stat_lyc_onoff].

    const bool lycEqLy = video.LYC == video.LY;

    const bool lycEqLyIrq =
        test_bit<LYC_EQ_LY_INTERRUPT>(video.STAT) && lycEqLy && dots != 454 /* LYC_EQ_LY is always 0 at dot 454*/;

    const uint8_t mode = keep_bits<2>(video.STAT);

    const bool hblankIrq = test_bit<HBLANK_INTERRUPT>(video.STAT) && mode == HBLANK;

    // VBlank interrupt is raised either with VBLANK or OAM STAT's flag when entering VBLANK.
    const bool vblankIrq =
        (test_bit<VBLANK_INTERRUPT>(video.STAT) || test_bit<OAM_INTERRUPT>(video.STAT)) && mode == VBLANK;

    // Eventually raise STAT interrupt
    updateStatIrq(lycEqLyIrq || hblankIrq || vblankIrq);

    // Update STAT's LYC_EQ_LY
    if (dots == 454 /* TODO: && keep_bits<2>(video.STAT) == HBLANK ?*/) {
        // LYC_EQ_LY is always 0 at dot 454 [mooneye: lcdon_timing]
        reset_bit<LYC_EQ_LY>(video.STAT);
    } else {
        // Update STAT's LYC=LY Flag according to the current comparison
        set_bit<LYC_EQ_LY>(video.STAT, lycEqLy);
    }
}

void Ppu::updateStatIrq(bool irq) {
    // Raise STAT interrupt request only on rising edge
    if (lastStatIrq < irq)
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Stat>();
    lastStatIrq = irq;
}

void Ppu::turnOn() {
    check(!on);
    check(video.LY == 0);

    on = true;

    // OAM bus is not acquired even if the PPU will proceed with OAM scan from here.

    // STAT's LYC_EQ_LY is updated properly (but interrupt is not raised)
    set_bit<LYC_EQ_LY>(video.STAT, video.LY == video.LYC);
}

void Ppu::turnOff() {
    check(on);
    on = false;
    dots = 0;
    video.LY = 0;
    lcd.reset();

    // clear oam entries eventually still there
    for (uint32_t i = 0; i < array_size(oamEntries); i++)
        oamEntries[i].clear();

    resetFetcher();

    IF_ASSERTS(oamEntriesCount = 0);
    IF_DEBUGGER(scanlineOamEntries.clear());

    tickSelector = &Ppu::oamScanEven;

    // STAT's mode goes to HBLANK, even if the PPU will start with OAM scan
    updateMode<HBLANK>();

    vram.release();
    oam.release();
}

void Ppu::enterOamScan() {
    checkCode({
        for (uint32_t i = 0; i < array_size(oamEntries); i++) {
            check(oamEntries[i].isEmpty());
        }
    });
    IF_ASSERTS(oamEntriesCount = 0);
    IF_DEBUGGER(scanlineOamEntries.clear());

    IF_DEBUGGER(timings.hBlank = 456 - (timings.oamScan + timings.pixelTransfer));

    oamScan.count = 0;

    tickSelector = &Ppu::oamScanEven;

    updateMode<OAM_SCAN>();

    check(!vram.isAcquiredByMe());
    oam.acquire();
}

void Ppu::oamScanEven() {
    check(dots % 2 == 0);
    check(oamScan.count < 10);
    check(oam.isAcquiredByMe() || keep_bits<2>(video.STAT) == HBLANK /* oam bus is not acquired after turn on */ ||
          dots == 76 /* oam bus seems released this cycle */);

    // Figure out oam number
    const uint8_t oamNumber = dots / 2;

    // Read two bytes from OAM (Y and X)
    readOamRegisters<OAM_SCAN>(OAM_ENTRY_BYTES * oamNumber);

    tickSelector = &Ppu::oamScanOdd;

    ++dots;
}

void Ppu::oamScanOdd() {
    check(dots % 2 == 1);
    check(oamScan.count < 10);
    check(oam.isAcquiredByMe() || keep_bits<2>(video.STAT) == HBLANK /* oam bus is not acquired after turn on */ ||
          dots == 77 /* oam bus seems released this cycle */);

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

    // Start of oddities
    if (dots == 76) {
        // OAM bus seems to be released (i.e. writes to OAM works normally) just for this cycle
        // [mooneye: lcdon_write_timing]
        if (keep_bits<2>(video.STAT) != HBLANK) {
            oam.release();
        }
    } else if (dots == 78) {
        if (keep_bits<2>(video.STAT) != HBLANK) {
            // OAM bus is re-acquired this cycle
            // [mooneye: lcdon_write_timing]
            oam.acquire();

            // VRAM bus is acquired one cycle before STAT is updated
            // [mooneye: lcdon_timing]
            vram.acquire();
        }
    }
    // End of oddities

    if (dots == 80) {
        enterPixelTransfer();
    } else {
        // Check if there is room for other oam entries in this scanline
        // if that's not the case, complete oam scan now
        tickSelector = (oamScan.count == 10) ? &Ppu::oamScanDone : &Ppu::oamScanEven;
    }
}

void Ppu::oamScanDone() {
    check(oamScan.count == 10);

    ++dots;

    // Start of oddities
    if (dots == 76) {
        // OAM bus seems to be released (writes to OAM works normally) just for this cycle
        // [mooneye: lcdon_write_timing]
        if (keep_bits<2>(video.STAT) != HBLANK) {
            oam.release();
        }
    } else if (dots == 78) {
        if (keep_bits<2>(video.STAT) != HBLANK) {
            // OAM bus is re-acquired this cycle
            // [mooneye: lcdon_write_timing]
            oam.acquire();

            // VRAM bus is acquired one cycle before STAT is updated
            // [mooneye: lcdon_timing]
            vram.acquire();
        }
    }
    // End of oddities

    if (dots == 80) {
        enterPixelTransfer();
    }
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
    pixelTransfer.initialSCX.toDiscard = mod<TILE_WIDTH>(video.SCX);
    pixelTransfer.initialSCX.discarded = 0;

    updateMode<PIXEL_TRANSFER>();

    vram.acquire();
    oam.acquire();
}

void Ppu::pixelTransferDummy0() {
    check(LX == 0);
    check(oam.isAcquiredByMe());
    check(vram.isAcquiredByMe());

    if (++dots == 83) {
        bgFifo.fill(DUMMY_TILE);
        tickSelector = pixelTransfer.initialSCX.toDiscard ? &Ppu::pixelTransferDiscard0 : &Ppu::pixelTransfer0;
    }
}

void Ppu::pixelTransferDiscard0() {
    check(LX == 0);
    check(pixelTransfer.initialSCX.toDiscard < 8);
    check(pixelTransfer.initialSCX.discarded < pixelTransfer.initialSCX.toDiscard);
    check(oam.isAcquiredByMe());
    check(vram.isAcquiredByMe());

    // the first SCX % 8 of a background tile are simply discarded
    // note that LX is not incremented in this case:
    // this let the obj align with the bg
    if (isBgFifoReadyToBePopped()) {
        bgFifo.popFront();

        if (++pixelTransfer.initialSCX.discarded == pixelTransfer.initialSCX.toDiscard)
            // all the SCX % 8 pixels have been discard
            tickSelector = &Ppu::pixelTransfer0;
    }

    tickFetcher();

    ++dots;
}

void Ppu::pixelTransfer0() {
    check(LX < 8);
    check(pixelTransfer.initialSCX.discarded == pixelTransfer.initialSCX.toDiscard);
    check(oam.isAcquiredByMe());
    check(vram.isAcquiredByMe());

    bool incLX = false;

    // for LX € [0, 8) just pop the pixels but do not push them to LCD
    if (isBgFifoReadyToBePopped()) {
        bgFifo.popFront();

        if (objFifo.isNotEmpty())
            objFifo.popFront();

        incLX = true;

        if (LX + 1 == 8)
            tickSelector = &Ppu::pixelTransfer8;

        eventuallySetupFetcherForWindow();
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

    // push a new pixel to the lcd if if the bg fifo contains
    // some pixels and it's not blocked by a sprite fetch
    if (isBgFifoReadyToBePopped()) {
        static constexpr uint8_t NO_COLOR = 4;
        uint8_t color {NO_COLOR};

        // pop out a pixel from BG fifo
        const BgPixel bgPixel = bgFifo.popFront();

        if (objFifo.isNotEmpty()) {
            const ObjPixel objPixel = objFifo.popFront();

            // take OBJ pixel instead of the BG pixel only if both are satisfied:
            // - it's opaque
            // - either BG_OVER_OBJ is disabled or the BG color is 0
            if (isObjOpaque(objPixel.colorIndex) &&
                (!test_bit<BG_OVER_OBJ>(objPixel.attributes) || bgPixel.colorIndex == 0)) {
                color = resolveColor(objPixel.colorIndex,
                                     test_bit<PALETTE_NUM>(objPixel.attributes) ? video.OBP1 : video.OBP0);
            }
        }

        if (color == NO_COLOR) {
            // There is 1 T-cycle delay between video.BGP and the BGP value the PPU sees.
            // During this T-Cycle, the PPU sees the last BGP ORed with the new BGP.
            // [mealybug/m3_bgp_change]
            const uint8_t BGP = (uint8_t)video.BGP | lastBGP;
            color = test_bit<BG_WIN_ENABLE>(video.LCDC) ? resolveColor(bgPixel.colorIndex, BGP) : 0;
        }

        check(color < NUMBER_OF_COLORS);

        lcd.pushPixel(static_cast<Lcd::Pixel>(color));

        incLX = true;

        if (LX + 1 == 168) {
            increaseLX();
            ++dots;
            enterHBlank();
            return;
        }

        eventuallySetupFetcherForWindow();
    }

    tickFetcher();

    if (incLX)
        increaseLX();

    ++dots;
}

void Ppu::enterHBlank() {
    check(LX == 168);
    check(video.LY < 144);

    // clang-format off
    checkCode({
        // Check Pixel Transfer Timing.

        static constexpr uint16_t LB = 80 + 3 + 21 * 8;                // 251

        // The minimum number of dots a pixel transfer can take is
        // -> 3 + 21 * 8 = 171.
        //
        // 3 cycles for the first dummy fetch
        // 8 cycles for push the first tile (that is simply discarded)
        // 8 * 20 cycles for push the 160 pixels.
        check(dots >= LB);

        // If SCX % 8 was > 0 the pixel transfer is delayed by SCX % 8 dots.
        // Furthermore, each window triggers delays pixel transfer by 6 dots.
        // Therefore, a more strict lower bound is eBgLB = 80 + 174 + SCX % 8 + 6 * WinTriggers
        const uint16_t eBgLB = LB + pixelTransfer.initialSCX.toDiscard + 6 * w.lineTriggers;
        check(dots >= eBgLB);

        // A sprite fetch requires at least 6 cycles, therefore a more strict lower bound is
        // -> eBgLB + 6 * oamEntriesCount
        check(dots >= eBgLB + 6 * oamEntriesCount);

        // The maximum number of dots a pixel transfer can take is
        // -> eBgLB + 10 * 11 + (7)
        //
        // 11 the maximum number of cycles a sprite can take
        // 10 the maximum number of sprites that can be rendered on a line
        check(dots <= eBgLB + 11 * 10);

        // More strictly: check based on the sprites actually there for this line
        check(dots <= eBgLB + 11 * oamEntriesCount);
    });
    // clang-format on

    IF_DEBUGGER(timings.pixelTransfer = dots - timings.oamScan);

    tickSelector = video.LY == 143 ? &Ppu::hBlankLastLine : &Ppu::hBlank;

    updateMode<HBLANK>();

    vram.release();
    oam.release();
}

void Ppu::hBlank() {
    check(LX == 168);
    check(video.LY < 143);
    if (++dots == 454) {
        // LY is increased at dot 454
        ++video.LY;

        tickSelector = &Ppu::hBlank454;

        // Eventually raise OAM interrupt
        updateStatIrq(test_bit<OAM_INTERRUPT>(video.STAT));

        oam.acquire();
    }
}

void Ppu::hBlank454() {
    if (++dots == 456) {
        dots = 0;
        enterOamScan();
    }
}

void Ppu::hBlankLastLine() {
    check(LX == 168);
    check(video.LY == 143);
    if (++dots == 454) {
        // LY is increased here: at dot 454 (not 456)
        ++video.LY;
        tickSelector = &Ppu::hBlankLastLine454;
    }
}

void Ppu::hBlankLastLine454() {
    check(dots == 454);

    ++dots;
    tickSelector = &Ppu::hBlankLastLine455;

    // VBlank mode is set at dot 455 (though I'm not sure about it)
    updateMode<VBLANK>();
}

void Ppu::hBlankLastLine455() {
    check(dots == 455);

    dots = 0;
    enterVBlank();
}

void Ppu::enterVBlank() {
    IF_DEBUGGER(timings.hBlank = 456 - timings.pixelTransfer);

    tickSelector = &Ppu::vBlank;

    check(((uint8_t)video.STAT & 0b11) == VBLANK);

    interrupts.raiseInterrupt<InterruptsIO::InterruptType::VBlank>();

    check(!vram.isAcquiredByMe());
    check(!oam.isAcquiredByMe());
}

void Ppu::vBlank() {
    check(video.LY >= 144 && video.LY < 154);
    if (++dots == 456) {
        dots = 0;
        // LY never reaches 154: it is set to 0 instead (which lasts 2 scanlines).
        video.LY = (video.LY + 1) % 153;
        if (video.LY == 0) {
            tickSelector = &Ppu::vBlankLastLine;
        }
    }
}

void Ppu::vBlankLastLine() {
    check(video.LY == 0);
    if (++dots == 454) {
        // It seems that STAT's mode is reset the last cycle (to investigate)
        updateMode<0>();
        tickSelector = &Ppu::vBlankLastLine454;
    }
}

void Ppu::vBlankLastLine454() {
    check(video.LY == 0);
    if (++dots == 456) {
        dots = 0;
        // end of vblank
        enterNewFrame();
    }
}

void Ppu::enterNewFrame() {
    // Reset window line counter
    // (reset to 255 so that it will go to 0 at the first window trigger).
    w.WLY = UINT8_MAX;

    enterOamScan();

    // Eventually raise OAM interrupt
    updateStatIrq(test_bit<OAM_INTERRUPT>(video.STAT));
}

template <uint8_t Mode>
inline void Ppu::updateMode() {
    check(Mode <= 0b11);
    video.STAT = ((uint8_t)video.STAT & 0b11111100) | Mode;
}

inline void Ppu::increaseLX() {
    // Clear oam entries of current LX (it might contain OBJs not served).
    // While doing so update oamEntriesCount so that it will contain
    // the real oam entries count.
    check(oamEntriesCount >= oamEntries[LX].size());
    IF_ASSERTS(oamEntriesCount -= oamEntries[LX].size());
    oamEntries[LX].clear();

    // Increase LX.
    ++LX;
}

inline bool Ppu::isBgFifoReadyToBePopped() const {
    // The condition for which the bg fifo can be popped are:
    // - the bg fifo should not be empty
    // - the fetcher is not fetching a sprite
    // - there are no more oam entries for this LX to be fetched or obj are disabled in LCDC
    return bgFifo.isNotEmpty() && !isFetchingSprite && (oamEntries[LX].isEmpty() || !test_bit<OBJ_ENABLE>(video.LCDC));
}

inline bool Ppu::isObjReadyToBeFetched() const {
    // The condition for which a obj fetch can be served are:
    // - there should be at least a pending oam entry for the current LX
    // - obj should be enabled in LCDC
    return oamEntries[LX].isNotEmpty() && test_bit<OBJ_ENABLE>(video.LCDC);
}

bool Ppu::isWindowTriggering() const {
    // The condition for which the window can be triggered are:
    // - current pixel transfer LX should match WX
    // - current LY should be greater than WY
    // - window should be enabled in LCDC
    return LX == video.WX && video.LY >= video.WY && test_bit<WIN_ENABLE>(video.LCDC);
}

inline void Ppu::eventuallySetupFetcherForWindow() {
    // Eventually reset the fetcher so that it fetches the window, if it is triggering now.
    if (!w.active && isWindowTriggering()) {
        w.active = true;

        // Store the triggering WX.
        w.WX = video.WX;

        // Increase the window line counter.
        ++w.WLY;

        IF_ASSERTS_OR_DEBUGGER(++w.lineTriggers);

        // Throw away the pixels in the BG fifo.
        bgFifo.clear();

        // Setup the fetcher to fetch a window tile
        fetcherTickSelector = &Ppu::winPrefetcherGetTile0;
    }
}

void Ppu::resetFetcher() {
    LX = 0;
    bgFifo.clear();
    objFifo.clear();
    isFetchingSprite = false;
    bwf.LX = 0;
    bwf.interruptedFetch.hasData = false;
    w.active = false;
    IF_ASSERTS_OR_DEBUGGER(w.lineTriggers = 0);
    fetcherTickSelector = &Ppu::bgwinPrefetcherGetTile0;
}

inline void Ppu::tickFetcher() {
    (this->*(fetcherTickSelector))();
}

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

    bwf.tilemapX = mod<TILEMAP_WIDTH>((bwf.LX + video.SCX) / TILE_WIDTH);

    fetcherTickSelector = &Ppu::bgPrefetcherGetTile1;
}

void Ppu::bgPrefetcherGetTile1() {
    check(!isFetchingSprite);

    const uint8_t SCY = video.SCY;
    const uint8_t LY = video.LY;
    const uint8_t LCDC = video.LCDC;
    const uint8_t tilemapY = mod<TILEMAP_HEIGHT>((LY + SCY) / TILE_HEIGHT);

    const uint16_t vTilemapAddr = (0x1800 | (test_bit<BG_TILE_MAP>(LCDC) << 10)); // 0x9800 or 0x9C00 (global)
    const uint16_t vTilemapTileAddr = vTilemapAddr + (TILEMAP_WIDTH * TILEMAP_CELL_BYTES * tilemapY) + bwf.tilemapX;
    const uint8_t tileNumber = vram.read(vTilemapTileAddr);

    const uint16_t vTileAddr = test_bit<BG_WIN_TILE_DATA>(LCDC)
                                   ?
                                   // unsigned addressing mode with 0x8000 as (global) base address
                                   0x0000 + TILE_BYTES * tileNumber
                                   :
                                   // signed addressing mode with 0x9000 as (global) base address
                                   0x1000 + TILE_BYTES * to_signed(tileNumber);

    const uint8_t tileY = mod<TILE_HEIGHT>(LY + SCY);

    psf.vTileDataAddress = vTileAddr + TILE_ROW_BYTES * tileY;

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataLow0;
}

void Ppu::winPrefetcherGetTile0() {
    check(!isFetchingSprite);
    bwf.tilemapX = (bwf.LX - (w.WX - 7)) / TILE_WIDTH;
    fetcherTickSelector = &Ppu::winPrefetcherGetTile1;
}

void Ppu::winPrefetcherGetTile1() {
    check(!isFetchingSprite);

    const uint8_t LCDC = video.LCDC;
    const uint8_t tilemapY = (w.WLY) / TILE_HEIGHT;

    const uint16_t vTilemapAddr = (0x1800 | (test_bit<WIN_TILE_MAP>(LCDC) << 10)); // 0x9800 or 0x9C00 (global)
    const uint16_t vTilemapTileAddr = vTilemapAddr + (TILEMAP_WIDTH * TILEMAP_CELL_BYTES * tilemapY) + bwf.tilemapX;
    const uint8_t tileNumber = vram.read(vTilemapTileAddr);

    const uint16_t vTileAddr = test_bit<BG_WIN_TILE_DATA>(LCDC)
                                   ?
                                   // unsigned addressing mode with 0x8000 as (global) base address
                                   0x0000 + TILE_BYTES * tileNumber
                                   :
                                   // signed addressing mode with 0x9000 as (global) base address
                                   0x1000 + TILE_BYTES * to_signed(tileNumber);

    const uint8_t tileY = mod<TILE_WIDTH>(w.WLY);

    psf.vTileDataAddress = vTileAddr + TILE_ROW_BYTES * tileY;

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataLow0;
}

void Ppu::objPrefetcherGetTile0() {
    check(isFetchingSprite);

    // Read two bytes from OAM (Tile Number and Attributes)
    readOamRegisters<PIXEL_TRANSFER>(OAM_ENTRY_BYTES * of.entry.number + Specs::Bytes::OAM::TILE_NUMBER);
    of.tileNumber = registers.oam.a;
    of.attributes = registers.oam.b;

    fetcherTickSelector = &Ppu::objPrefetcherGetTile1;
}

void Ppu::objPrefetcherGetTile1() {
    check(isFetchingSprite);

    const bool isDoubleHeight = test_bit<OBJ_SIZE>(video.LCDC);
    if (isDoubleHeight)
        // bit 0 of tile number is ignored for 8x16 obj
        reset_bit<0>(of.tileNumber);

    const uint16_t vTileAddr = TILE_BYTES * of.tileNumber;

    const int32_t objY = of.entry.y - TILE_DOUBLE_HEIGHT;
    check(video.LY >= objY);

    uint8_t tileY = video.LY - objY;
    check(tileY < (TILE_HEIGHT << isDoubleHeight));

    if (test_bit<Y_FLIP>(of.attributes)) {
        // take the opposite row if Y_FLIP flag is set
        const uint8_t objHeight = TILE_HEIGHT << isDoubleHeight;
        tileY = objHeight - tileY - 1;
    }

    psf.vTileDataAddress = vTileAddr + TILE_ROW_BYTES * tileY;

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataLow0;
}

void Ppu::bgwinPixelSliceFetcherGetTileDataLow0() {
    check(!isFetchingSprite);

    psf.tileDataLow = vram.read(psf.vTileDataAddress);

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataLow1;
}

void Ppu::bgwinPixelSliceFetcherGetTileDataLow1() {
    check(!isFetchingSprite);

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataHigh0;
}

void Ppu::bgwinPixelSliceFetcherGetTileDataHigh0() {
    check(!isFetchingSprite);

    psf.tileDataHigh = vram.read(psf.vTileDataAddress + 1);

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1;
}

void Ppu::bgwinPixelSliceFetcherGetTileDataHigh1() {
    check(!isFetchingSprite);

    // if there is a pending obj hit (and the bg fifo is not empty),
    // discard the fetched bg pixels and restart the obj prefetcher with the obj hit
    // note: the first obj prefetcher tick should overlap this tick, not the push one
    if (isObjReadyToBeFetched() && bgFifo.isNotEmpty()) {
        // the bg/win tile fetched is not thrown away: instead it is
        // cached so that the fetcher can start with it after the sprite
        // has been merged into obj fifo
        cacheBgWinFetch();

        isFetchingSprite = true;
        of.entry = oamEntries[LX].popBack();
        objPrefetcherGetTile0();
        return;
    }

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherPush;
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

    // the pixels can be pushed only if the bg fifo is empty,
    // otherwise wait in push state until bg fifo is emptied
    const bool canPushToBgFifo = bgFifo.isEmpty();
    if (canPushToBgFifo) {
        // push pixels into bg fifo
        bgFifo.fill(&TILE_ROW_DATA_TO_ROW_PIXELS[psf.tileDataHigh << 8 | psf.tileDataLow]);

        // advance to next tile
        bwf.LX += TILE_WIDTH; // automatically handle mod 8 by overflowing

        fetcherTickSelector = &Ppu::bgwinPrefetcherGetTile0;
    }

    // if there is a pending obj hit, discard the fetched bg pixels
    // and restart the obj prefetcher with the obj hit
    // note: the first obj prefetcher tick overlap this tick
    if (isObjReadyToBeFetched()) {
        if (!canPushToBgFifo) {
            // the bg/win tile fetched is not thrown away: instead it is
            // cached so that the fetcher can start with it after the sprite
            // has been merged into obj fifo
            cacheBgWinFetch();
        }

        isFetchingSprite = true;
        of.entry = oamEntries[LX].popBack();
        objPrefetcherGetTile0();
        return;
    }
}

void Ppu::objPixelSliceFetcherGetTileDataLow0() {
    check(isFetchingSprite);
    check(bgFifo.isNotEmpty());

    psf.tileDataLow = vram.read(psf.vTileDataAddress);

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataLow1;
}

void Ppu::objPixelSliceFetcherGetTileDataLow1() {
    check(isFetchingSprite);
    check(bgFifo.isNotEmpty());

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataHigh0;
}

void Ppu::objPixelSliceFetcherGetTileDataHigh0() {
    check(isFetchingSprite);
    check(bgFifo.isNotEmpty());

    psf.tileDataHigh = vram.read(psf.vTileDataAddress + 1);

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo;
}

void Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo() {
    check(isFetchingSprite);
    check(of.entry.x == LX);
    check(bgFifo.isNotEmpty());

    PixelColorIndex objPixelsColors[8];
    const uint8_t(*pixelsMapPtr)[8] =
        test_bit<X_FLIP>(of.attributes) ? TILE_ROW_DATA_TO_ROW_PIXELS_FLIPPED : TILE_ROW_DATA_TO_ROW_PIXELS;
    memcpy(objPixelsColors, &pixelsMapPtr[psf.tileDataHigh << 8 | psf.tileDataLow], 8);

    ObjPixel objPixels[8];
    for (uint8_t i = 0; i < 8; i++) {
        objPixels[i] = {
            .colorIndex = objPixelsColors[i], .attributes = of.attributes, .number = of.entry.number, .x = LX};
    }

    // handle sprite-to-sprite conflicts by merging the new obj pixels with
    // the pixels already in the obj fifo
    // "The smaller the X coordinate, the higher the priority."
    // "When X coordinates are identical, the object located first in OAM has higher priority."

    const uint8_t objFifoSize = objFifo.size();
    uint8_t i = 0;

    // handle conflict between the new and the old obj pixels
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

    // push the remaining obj pixels
    while (i < 8) {
        objFifo.pushBack(objPixels[i++]);
    }

    check(objFifo.isFull());

    if (isObjReadyToBeFetched()) {
        // still oam entries hit to be served for this x: setup the fetcher
        // for another obj fetch
        of.entry = oamEntries[LX].popBack();
        fetcherTickSelector = &Ppu::objPrefetcherGetTile0;
    } else {
        // no more oam entries to serve for this x: setup to fetcher with
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

template <uint8_t Mode>
void Ppu::readOamRegisters(uint16_t oamAddress) {
    static_assert(Mode == OAM_SCAN || Mode == PIXEL_TRANSFER);
    check(oamAddress % 2 == 0);

    if constexpr (Mode == OAM_SCAN) {
        // PPU cannot access OAM while DMA transfer is in progress.
        // Note that it does not read at all: therefore the oam registers
        // will hold their old values in this case.
        if (oam.isAcquiredBy<Device::Dma>())
            return;
    }

    // We can read from OAM.
    // Note that if DMA transfer is in progress conflicts can occur and
    // we might end up reading from the OAM address the DMA is writing to.
    // (but only if DMA writing request is pending, i.e. it is in t0 or t1)
    // [hacktix/strikethrough]
    const auto res = oam.readTwo(oamAddress);
    registers.oam.a = res.a;
    registers.oam.b = res.b;
}

void Ppu::saveState(Parcel& parcel) const {
    static constexpr TickSelector TICK_SELECTORS[] {&Ppu::oamScanEven,
                                                    &Ppu::oamScanOdd,
                                                    &Ppu::oamScanDone,
                                                    &Ppu::pixelTransferDummy0,
                                                    &Ppu::pixelTransferDiscard0,
                                                    &Ppu::pixelTransfer0,
                                                    &Ppu::pixelTransfer8,
                                                    &Ppu::hBlank,
                                                    &Ppu::hBlank454,
                                                    &Ppu::hBlankLastLine,
                                                    &Ppu::hBlankLastLine454,
                                                    &Ppu::vBlank,
                                                    &Ppu::vBlankLastLine,
                                                    &Ppu::vBlankLastLine454};

    static constexpr TickSelector FETCHER_TICK_SELECTORS[] {
        &Ppu::bgwinPrefetcherGetTile0,
        &Ppu::bgPrefetcherGetTile0,
        &Ppu::bgPrefetcherGetTile1,
        &Ppu::winPrefetcherGetTile0,
        &Ppu::winPrefetcherGetTile1,
        &Ppu::objPrefetcherGetTile0,
        &Ppu::objPrefetcherGetTile1,
        &Ppu::bgwinPixelSliceFetcherGetTileDataLow0,
        &Ppu::bgwinPixelSliceFetcherGetTileDataLow1,
        &Ppu::bgwinPixelSliceFetcherGetTileDataHigh0,
        &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1,
        &Ppu::bgwinPixelSliceFetcherPush,
        &Ppu::objPixelSliceFetcherGetTileDataLow0,
        &Ppu::objPixelSliceFetcherGetTileDataLow1,
        &Ppu::objPixelSliceFetcherGetTileDataHigh0,
        &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo};

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
    parcel.writeUInt16(dots);
    parcel.writeUInt8(LX);
    parcel.writeUInt8(lastBGP);

    parcel.writeBytes(bgFifo.data, sizeof(bgFifo.data));
    parcel.writeUInt8(bgFifo.cursor);

    parcel.writeBytes(objFifo.data, sizeof(objFifo.data));
    parcel.writeUInt8(objFifo.start);
    parcel.writeUInt8(objFifo.end);
    parcel.writeUInt8(objFifo.count);

    parcel.writeBytes(oamEntries, sizeof(oamEntries));
    IF_ASSERTS(parcel.writeUInt8(oamEntriesCount));

    parcel.writeBool(isFetchingSprite);

    parcel.writeUInt8(registers.oam.a);
    parcel.writeUInt8(registers.oam.b);

    parcel.writeUInt8(oamScan.count);

    parcel.writeUInt8(pixelTransfer.initialSCX.toDiscard);
    parcel.writeUInt8(pixelTransfer.initialSCX.discarded);

    parcel.writeUInt8(bwf.LX);
    parcel.writeUInt8(bwf.tilemapX);
    parcel.writeUInt8(bwf.interruptedFetch.tileDataLow);
    parcel.writeUInt8(bwf.interruptedFetch.tileDataHigh);
    parcel.writeBool(bwf.interruptedFetch.hasData);

    parcel.writeUInt8(w.WLY);
    parcel.writeUInt8(w.WX);
    parcel.writeBool(w.active);
    IF_ASSERTS_OR_DEBUGGER(parcel.writeUInt8(w.lineTriggers));

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
    static constexpr TickSelector TICK_SELECTORS[] {&Ppu::oamScanEven,
                                                    &Ppu::oamScanOdd,
                                                    &Ppu::oamScanDone,
                                                    &Ppu::pixelTransferDummy0,
                                                    &Ppu::pixelTransferDiscard0,
                                                    &Ppu::pixelTransfer0,
                                                    &Ppu::pixelTransfer8,
                                                    &Ppu::hBlank,
                                                    &Ppu::hBlank454,
                                                    &Ppu::hBlankLastLine,
                                                    &Ppu::hBlankLastLine454,
                                                    &Ppu::vBlank,
                                                    &Ppu::vBlankLastLine};

    static constexpr TickSelector FETCHER_TICK_SELECTORS[] {
        &Ppu::bgwinPrefetcherGetTile0,
        &Ppu::bgPrefetcherGetTile0,
        &Ppu::bgPrefetcherGetTile1,
        &Ppu::winPrefetcherGetTile0,
        &Ppu::winPrefetcherGetTile1,
        &Ppu::objPrefetcherGetTile0,
        &Ppu::objPrefetcherGetTile1,
        &Ppu::bgwinPixelSliceFetcherGetTileDataLow0,
        &Ppu::bgwinPixelSliceFetcherGetTileDataLow1,
        &Ppu::bgwinPixelSliceFetcherGetTileDataHigh0,
        &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1,
        &Ppu::bgwinPixelSliceFetcherPush,
        &Ppu::objPixelSliceFetcherGetTileDataLow0,
        &Ppu::objPixelSliceFetcherGetTileDataLow1,
        &Ppu::objPixelSliceFetcherGetTileDataHigh0,
        &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo};

    const uint8_t tickSelectorNumber = parcel.readUInt8();
    check(tickSelectorNumber < array_size(TICK_SELECTORS));
    tickSelector = TICK_SELECTORS[tickSelectorNumber];

    const uint8_t fetcherTickSelectorNumber = parcel.readUInt8();
    check(fetcherTickSelectorNumber < array_size(FETCHER_TICK_SELECTORS));
    fetcherTickSelector = FETCHER_TICK_SELECTORS[fetcherTickSelectorNumber];

    on = parcel.readBool();
    lastStatIrq = parcel.readBool();
    dots = parcel.readUInt16();
    LX = parcel.readUInt8();
    lastBGP = parcel.readUInt8();

    parcel.readBytes(bgFifo.data, sizeof(bgFifo.data));
    bgFifo.cursor = parcel.readUInt8();

    parcel.readBytes(objFifo.data, sizeof(objFifo.data));
    objFifo.start = parcel.readUInt8();
    objFifo.end = parcel.readUInt8();
    objFifo.count = parcel.readUInt8();

    parcel.readBytes(oamEntries, sizeof(oamEntries));
    IF_ASSERTS(oamEntriesCount = parcel.readUInt8());

    isFetchingSprite = parcel.readBool();

    registers.oam.a = parcel.readUInt8();
    registers.oam.b = parcel.readUInt8();

    oamScan.count = parcel.readUInt8();

    pixelTransfer.initialSCX.toDiscard = parcel.readUInt8();
    pixelTransfer.initialSCX.discarded = parcel.readUInt8();

    bwf.LX = parcel.readUInt8();
    bwf.tilemapX = parcel.readUInt8();
    bwf.interruptedFetch.tileDataLow = parcel.readUInt8();
    bwf.interruptedFetch.tileDataHigh = parcel.readUInt8();
    bwf.interruptedFetch.hasData = parcel.readBool();

    w.WLY = parcel.readUInt8();
    w.WX = parcel.readUInt8();
    w.active = parcel.readBool();
    IF_ASSERTS_OR_DEBUGGER(w.lineTriggers = parcel.readUInt8());

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