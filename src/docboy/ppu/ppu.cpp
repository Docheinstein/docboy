#include "ppu.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/lcd/lcd.h"
#include "docboy/memory/memory.hpp"
#include "docboy/ppu/video.h"
#include "pixelmap.h"
#include "utils/casts.hpp"
#include "utils/macros.h"
#include "utils/math.h"

using namespace Specs::Bits::Video::LCDC;
using namespace Specs::Bits::Video::STAT;
using namespace Specs::Bits::OAM::Attributes;

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

Ppu::Ppu(Lcd& lcd, VideoIO& video, InterruptsIO& interrupts, Vram& vram, Oam& oam) :
    lcd(lcd),
    video(video),
    interrupts(interrupts),
    vram(vram),
    oam(oam) {
}

void Ppu::tick() {
    const bool enabled = test_bit<LCD_ENABLE>(video.LCDC);

    if (on != enabled) {
        enabled ? turnOn() : turnOff();
    }

    if (!on)
        return;

    (this->*(tickSelector))();

    check(dots < 456);

    IF_DEBUGGER(++cycles);
}

void Ppu::turnOn() {
    check(!on);
    on = true;

    // TODO: needed? how to do it without mode?
    // lcdIo.writeSTAT(reset_bits<2>(lcdIo.readSTAT()) | state);
    //    video.STAT = ((uint8_t) video.STAT & 0b11111100) | mode;
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
    enterOamScan();

    video.STAT = discard_bits<2>(video.STAT);
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

    tickSelector = &Ppu::oamScan0;

    updateStat<Specs::Ppu::Modes::OAM_SCAN>();

    if (test_bit<OAM_INTERRUPT>(video.STAT))
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Stat>();
}

void Ppu::oamScan0() {
    check(oamScan.count < 10);
    check(dots % 2 == 0);

    // figure out oam number
    oamScan.entry.number = dots / 2;

    // read oam y
    oamScan.entry.y = oam[OAM_ENTRY_BYTES * oamScan.entry.number + Specs::Bytes::OAM::Y];

    tickSelector = &Ppu::oamScan1;

    ++dots;
}

void Ppu::oamScan1() {
    check(oamScan.count < 10);
    check(dots % 2 == 1);

#define ADVANCE_TO_OAM_MODE_OR_PIXEL_TRANSFER(oamStatePtr)                                                             \
    do {                                                                                                               \
        if (++dots == 80)                                                                                              \
            enterPixelTransfer();                                                                                      \
        else                                                                                                           \
            tickSelector = oamStatePtr;                                                                                \
    } while (0)

    // read oam entry height
    const uint8_t objHeight = TILE_WIDTH << test_bit<OBJ_SIZE>(video.LCDC);

    // check if the sprite is upon this scanline
    const uint8_t LY = video.LY;
    const int32_t objY = oamScan.entry.y - TILE_DOUBLE_HEIGHT;

    if (objY <= LY && LY < objY + objHeight) {
        // read oam entry x
        const uint8_t oamEntryX = oam[OAM_ENTRY_BYTES * oamScan.entry.number + Specs::Bytes::OAM::X];

        // push oam entry
        if (oamEntryX < 168) {
            oamEntries[oamEntryX].emplaceBack(static_cast<uint8_t>(dots / 2),
                                              oamScan.entry.y IF_ASSERTS_OR_DEBUGGER(COMMA oamEntryX));
            IF_DEBUGGER(scanlineOamEntries.emplaceBack(static_cast<uint8_t>(dots / 2),
                                                       oamScan.entry.y IF_ASSERTS_OR_DEBUGGER(COMMA oamEntryX)));
            IF_ASSERTS(++oamEntriesCount);
        }

        // check if there is room for other oam entries in this scanline
        // if that's not the case, complete oam scan now
        if (++oamScan.count == 10) {
            ADVANCE_TO_OAM_MODE_OR_PIXEL_TRANSFER(&Ppu::oamScanCompleted);
            return;
        }
    }

    check(oamScan.count < 10);

    ADVANCE_TO_OAM_MODE_OR_PIXEL_TRANSFER(&Ppu::oamScan0);

#undef ADVANCE_TO_OAM_MODE_OR_PIXEL_TRANSFER
}

void Ppu::oamScanCompleted() {
    check(oamScan.count == 10);

    if (++dots == 80) {
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

    if (test_bit<WIN_ENABLE>(video.LCDC) && video.WX == 7 && video.LY >= video.WY) {
        // the first bg fetch of this scanline is for the window
        // do not discard any pixel due to SCX
        tickSelector = &Ppu::pixelTransferDummy0<false>;
        IF_ASSERTS(firstFetchWasBg = false);
    } else {
        // the first bg fetch of this scanline is for the bg
        // eventually discard SCX % 8 pixels
        tickSelector = &Ppu::pixelTransferDummy0<true>;
        bgPixelTransfer.SCXmod8 = mod<TILE_WIDTH>(video.SCX);
        bgPixelTransfer.discardedPixels = 0;
        IF_ASSERTS(firstFetchWasBg = true);
    }

    updateStat<Specs::Ppu::Modes::PIXEL_TRANSFER>();
}

template <bool Bg>
void Ppu::pixelTransferDummy0() {
    check(LX == 0);
    // TODO: Pandocs says this lasts 4 dots, while Nitty Gritty says 6 docs.
    //       Figure out which is right: right now I use 4 since this passes mooneye intr_2_0_timing
    if (++dots == 84) {
        bgFifo.fill(DUMMY_TILE);
        if constexpr (Bg) {
            if (bgPixelTransfer.SCXmod8)
                tickSelector = &Ppu::pixelTransferDiscard0;
            else
                tickSelector = &Ppu::pixelTransfer0;
        } else {
            tickSelector = &Ppu::pixelTransfer0;
        }
    }
}

void Ppu::pixelTransferDiscard0() {
    check(LX == 0);
    check(bgPixelTransfer.SCXmod8 < 8);
    check(bgPixelTransfer.discardedPixels < bgPixelTransfer.SCXmod8);

    // the first SCX % 8 of a background tile are simply discarded
    // note that LX is not incremented in this case:
    // this let the obj align with the bg
    if (!isPixelTransferBlocked() && bgFifo.isNotEmpty()) {
        bgFifo.popFront();

        if (++bgPixelTransfer.discardedPixels == bgPixelTransfer.SCXmod8)
            // all the SCX % 8 pixels have been discard
            tickSelector = &Ppu::pixelTransfer0;
    }

    tickFetcher();

    ++dots;
}

void Ppu::pixelTransfer0() {
    check(LX < 8);
    check(!firstFetchWasBg || bgPixelTransfer.discardedPixels == bgPixelTransfer.SCXmod8);

    uint8_t nextLX = LX;

    // for LX € [0, 8) just pop the pixels but do not push them to LCD
    if (!isPixelTransferBlocked() && bgFifo.isNotEmpty()) {
        bgFifo.popFront();

        if (objFifo.isNotEmpty())
            objFifo.popFront();

        nextLX = LX + 1;

        if (nextLX == 8)
            tickSelector = &Ppu::pixelTransfer8;
    }

    tickFetcher();

    LX = nextLX;

    ++dots;
}

void Ppu::pixelTransfer8() {
    check(LX >= 8);

    uint8_t nextLX = LX;

    // push a new pixel to the lcd if if the bg fifo contains
    // some pixels and it's not blocked by a sprite fetch
    if (!isPixelTransferBlocked() && bgFifo.isNotEmpty()) {
        static constexpr uint8_t NO_COLOR = 4;
        uint8_t color {NO_COLOR};

        // pop out a pixel from BG fifo
        const BgPixel bgPixel = bgFifo.popFront();

        if (objFifo.isNotEmpty()) {
            const ObjPixel objPixel = objFifo.popFront();

            // take OBJ pixel instead of the BG pixel only if both are satisfied:
            // 1) OBJ_ENABLE is set
            // 2) it's opaque
            // 3) either BG_OVER_OBJ is disabled or the BG color is 0
            if (test_bit<OBJ_ENABLE>(video.LCDC) && isObjOpaque(objPixel.colorIndex) &&
                (!test_bit<BG_OVER_OBJ>(objPixel.attributes) || bgPixel.colorIndex == 0)) {
                color = resolveColor(objPixel.colorIndex,
                                     test_bit<PALETTE_NUM>(objPixel.attributes) ? video.OBP1 : video.OBP0);
            }
        }

        if (color == NO_COLOR) {
            color = test_bit<BG_WIN_ENABLE>(video.LCDC) ? resolveColor(bgPixel.colorIndex, video.BGP) : 0;
        }

        check(color < NUMBER_OF_COLORS);

        lcd.pushPixel(static_cast<Lcd::Pixel>(color));

        nextLX = LX + 1;

        if (nextLX == 168) {
            LX = 168;
            ++dots;
            enterHBlank();
            return;
        }
    }

    tickFetcher();

    LX = nextLX;

    ++dots;
}

void Ppu::enterHBlank() {
    check(LX == 168);
    check(video.LY < 144);

    // clang-format off
    checkCode({
        // Check Pixel Transfer Timing.

        static constexpr uint16_t LB = 80 + 4 + 21 * 8;                // 252
        static constexpr uint16_t UB = LB + 11 * 10 + 7 /* window? */; // 369

        // The minimum number of dots a pixel transfer can take is
        // -> 6 * 21 * 8 = 174.
        //
        // 6 cycles for the first dummy fetch
        // 8 cycles for push the first tile (that is simply discarded)
        // 8 * 20 cycles for push the 160 pixels.
        check(dots >= LB);

        // The maximum number of dots a pixel transfer can take is
        // -> LB + 11 * 10 + 7 = 291.
        //
        // 11 the maximum number of cycles a sprite can take
        // 10 the maximum number of sprites that can be rendered on a line
        // 7  window penalty
        check(dots <= UB);

        // If the BG has been rendered and SCX % 8 was > 0,
        // then pixel transfer is delayed by SCX % 8 dots.

        // Therefore a more strict lower bound is eBgLB = 80 + 174 + SCX % 8
        const uint16_t eBgLB = LB + (firstFetchWasBg ? bgPixelTransfer.SCXmod8 : 0);
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

        // TODO: consider also (7) window penalty?
        // TODO: check timing based on the expected penalty for the sprites hit
    });
    // clang-format on

    IF_DEBUGGER(timings.pixelTransfer = dots - timings.oamScan);

    // eventually increase window line counter if the window
    // has been displayed at least once in this scanline
    w.WLY += w.shown;
    w.shown = false;

    tickSelector = &Ppu::hBlank;

    updateStat<Specs::Ppu::Modes::HBLANK>();

    if (test_bit<HBLANK_INTERRUPT>(video.STAT))
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Stat>();
}

void Ppu::hBlank() {
    check(LX == 168);
    check(video.LY < 144);
    if (++dots == 456) {
        dots = 0;
        const uint8_t nextLY = (video.LY + 1);
        writeLY(nextLY);
        if (nextLY == 144)
            enterVBlank();
        else {
            enterOamScan();
        }
    }
}

void Ppu::enterVBlank() {
    IF_DEBUGGER(timings.hBlank = 456 - timings.pixelTransfer);

    tickSelector = &Ppu::vBlank;

    updateStat<Specs::Ppu::Modes::VBLANK>();

    if (test_bit<VBLANK_INTERRUPT>(video.STAT))
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Stat>();

    interrupts.raiseInterrupt<InterruptsIO::InterruptType::VBlank>();
}

void Ppu::vBlank() {
    check(video.LY >= 144 && video.LY < 154);
    if (++dots == 456) {
        dots = 0;
        const uint8_t nextLY = (video.LY + 1) % 154;
        writeLY(nextLY);
        if (nextLY == 0) {
            // end of vblank
            enterNewFrame();
        }
    }
}

void Ppu::enterNewFrame() {
    // reset window line counter
    w.WLY = 0;
    enterOamScan();
}

template <uint8_t Mode>
inline void Ppu::updateStat() {
    check(Mode <= 0b11);
    video.STAT = ((uint8_t)video.STAT & 0b11111100) | Mode;
}

void Ppu::writeLY(uint8_t LY) {
    check(LY < 154);

    // write LY
    video.LY = LY;

    const bool LY_equals_LYC = LY == video.LYC;

    // update STAT's LYC=LY Flag
    set_bit<LYC_EQ_LY>(video.STAT, LY_equals_LYC);

    // eventually raise STAT interrupt
    if (LY_equals_LYC && test_bit<LYC_EQ_LY_INTERRUPT>(video.STAT)) {
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Stat>();
    }
}

inline bool Ppu::isPixelTransferBlocked() const {
    // the pixel transfer is stalled if the fetcher is either fetching a sprite
    // or if there's a obj hit for this x that is ready to be fetched
    return isFetchingSprite || oamEntries[LX].isNotEmpty();
}

void Ppu::resetFetcher() {
    LX = 0;
    bgFifo.clear();
    objFifo.clear();
    isFetchingSprite = false;
    bwf.LX = 0;
    fetcherTickSelector = &Ppu::bgwinPrefetcherGetTile0;
}

inline void Ppu::tickFetcher() {
    (this->*(fetcherTickSelector))();
}

void Ppu::bgwinPrefetcherGetTile0() {
    check(!isFetchingSprite);

    // check whether fetch the tile for the window or for the bg
    if (test_bit<WIN_ENABLE>(video.LCDC)) {
        const uint8_t WX = video.WX;

        // TODO: figure out what actually happens if WX - 7 % 8 != 0: is the window just not shown?

        //        if (mod2<TILE_WIDTH>(WX - 7) == 0) {
        // TODO: is the check above needed or not?
        if (bwf.LX >= WX - 7 && video.LY >= video.WY) {
            winPrefetcherGetTile0();
            w.shown = true;
            return;
        }
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
    const uint8_t tileNumber = vram[vTilemapTileAddr];

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

    bwf.tilemapX = (bwf.LX - (video.WX - 7)) / TILE_WIDTH;

    fetcherTickSelector = &Ppu::winPrefetcherGetTile1;
}

void Ppu::winPrefetcherGetTile1() {
    check(!isFetchingSprite);

    const uint8_t LCDC = video.LCDC;
    const uint8_t tilemapY = (w.WLY) / TILE_HEIGHT;

    const uint16_t vTilemapAddr = (0x1800 | (test_bit<WIN_TILE_MAP>(LCDC) << 10)); // 0x9800 or 0x9C00 (global)
    const uint16_t vTilemapTileAddr = vTilemapAddr + (TILEMAP_WIDTH * TILEMAP_CELL_BYTES * tilemapY) + bwf.tilemapX;
    const uint8_t tileNumber = vram[vTilemapTileAddr];

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

    of.tileNumber = oam[OAM_ENTRY_BYTES * of.entry.number + Specs::Bytes::OAM::TILE_NUMBER];

    fetcherTickSelector = &Ppu::objPrefetcherGetTile1;
}

void Ppu::objPrefetcherGetTile1() {
    check(isFetchingSprite);

    of.attributes = oam[OAM_ENTRY_BYTES * of.entry.number + Specs::Bytes::OAM::ATTRIBUTES];

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

    psf.tileDataLow = vram[psf.vTileDataAddress];

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataLow1;
}

void Ppu::bgwinPixelSliceFetcherGetTileDataLow1() {
    check(!isFetchingSprite);

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataHigh0;
}

void Ppu::bgwinPixelSliceFetcherGetTileDataHigh0() {
    check(!isFetchingSprite);

    psf.tileDataHigh = vram[psf.vTileDataAddress + 1];

    fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1;
}

void Ppu::bgwinPixelSliceFetcherGetTileDataHigh1() {
    check(!isFetchingSprite);

    // if there is a pending obj hit, discard the fetched bg pixels
    // and restart the obj prefetcher with the obj hit
    // note: the first obj prefetcher tick should overlap this tick, not the push one
    if (oamEntries[LX].isNotEmpty() && bgFifo.isNotEmpty()) {
        // the bg/win tile fetched is not thrown away: instead it is
        // cached so that the fetcher can start with it after the sprite
        // has been merged into obj fifo
        cacheInterruptedBgWinFetch();

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
     * As far as I'm understanding, there are 4 possible cases
     * bgFifo.IsEmpty() | oamEntriesHit.isEmpty()
     *        0         |           0              | cache bg fetch and start obj prefetcher (tick now)
     *        0         |           1              | wait (nop)
     *        1         |           0              | push to bg fifo and start obj prefetcher (tick now)
     *        1         |           1              | push to bg fifo and prepare for bg/win prefetcher (tick next dot)
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
    if (oamEntries[LX].isNotEmpty()) {
        if (!canPushToBgFifo) {
            // the bg/win tile fetched is not thrown away: instead it is
            // cached so that the fetcher can start with it after the sprite
            // has been merged into obj fifo
            cacheInterruptedBgWinFetch();
        }

        isFetchingSprite = true;
        of.entry = oamEntries[LX].popBack();
        objPrefetcherGetTile0();
        return;
    }
}

void Ppu::objPixelSliceFetcherGetTileDataLow0() {
    check(isFetchingSprite);

    psf.tileDataLow = vram[psf.vTileDataAddress];

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataLow1;
}

void Ppu::objPixelSliceFetcherGetTileDataLow1() {
    check(isFetchingSprite);

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataHigh0;
}

void Ppu::objPixelSliceFetcherGetTileDataHigh0() {
    check(isFetchingSprite);

    psf.tileDataHigh = vram[psf.vTileDataAddress + 1];

    fetcherTickSelector = &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo;
}

void Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo() {
    check(isFetchingSprite);
    check(of.entry.x == LX);

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

    if (oamEntries[LX].isNotEmpty() && bgFifo.isNotEmpty()) {
        // still oam entries hit to be served for this x: setup the fetcher
        // for another obj fetch
        of.entry = oamEntries[LX].popBack();
        fetcherTickSelector = &Ppu::objPrefetcherGetTile0;
    } else {
        // no more oam entries to serve for this x: setup to fetcher with
        // the cached tile data that has been interrupted by this obj fetch
        isFetchingSprite = false;

        if (bwf.interruptedFetch.hasData) {
            restoreInterruptedBgWinFetch();
            fetcherTickSelector = &Ppu::bgwinPixelSliceFetcherPush;
        } else {
            fetcherTickSelector = &Ppu::bgwinPrefetcherGetTile0;
        }
    }
}

inline void Ppu::cacheInterruptedBgWinFetch() {
    bwf.interruptedFetch.tileDataLow = psf.tileDataLow;
    bwf.interruptedFetch.tileDataHigh = psf.tileDataHigh;
    check(!bwf.interruptedFetch.hasData);
    bwf.interruptedFetch.hasData = true;
}

inline void Ppu::restoreInterruptedBgWinFetch() {
    psf.tileDataLow = bwf.interruptedFetch.tileDataLow;
    psf.tileDataHigh = bwf.interruptedFetch.tileDataHigh;
    check(bwf.interruptedFetch.hasData);
    bwf.interruptedFetch.hasData = false;
}

void Ppu::saveState(Parcel& parcel) const {
    static constexpr TickSelector TICK_SELECTORS[] {&Ppu::oamScan0,
                                                    &Ppu::oamScan1,
                                                    &Ppu::oamScanCompleted,
                                                    &Ppu::pixelTransferDummy0<false>,
                                                    &Ppu::pixelTransferDummy0<true>,
                                                    &Ppu::pixelTransferDiscard0,
                                                    &Ppu::pixelTransfer0,
                                                    &Ppu::pixelTransfer8,
                                                    &Ppu::hBlank,
                                                    &Ppu::vBlank};

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
    parcel.writeUInt16(dots);
    parcel.writeUInt8(LX);

    parcel.writeBytes(bgFifo.data, sizeof(bgFifo.data));
    parcel.writeUInt8(bgFifo.cursor);

    parcel.writeBytes(objFifo.data, sizeof(objFifo.data));
    parcel.writeUInt8(objFifo.start);
    parcel.writeUInt8(objFifo.end);
    parcel.writeUInt8(objFifo.count);

    parcel.writeBytes(oamEntries, sizeof(oamEntries));
    IF_ASSERTS(parcel.writeUInt8(oamEntriesCount));

    parcel.writeBool(isFetchingSprite);

    parcel.writeUInt8(oamScan.count);
    parcel.writeUInt8(oamScan.entry.number);
    parcel.writeUInt8(oamScan.entry.y);

    parcel.writeUInt8(bgPixelTransfer.SCXmod8);
    parcel.writeUInt8(bgPixelTransfer.discardedPixels);

    parcel.writeUInt8(bwf.LX);
    parcel.writeUInt8(bwf.tilemapX);
    parcel.writeUInt8(bwf.interruptedFetch.tileDataLow);
    parcel.writeUInt8(bwf.interruptedFetch.tileDataHigh);
    parcel.writeBool(bwf.interruptedFetch.hasData);

    parcel.writeUInt8(w.WLY);
    parcel.writeBool(w.shown);

    parcel.writeUInt8(of.entry.number);
    parcel.writeUInt8(of.entry.y);
    IF_ASSERTS(parcel.writeUInt8(of.entry.x));
    parcel.writeUInt8(of.tileNumber);
    parcel.writeUInt8(of.attributes);

    parcel.writeUInt16(psf.vTileDataAddress);
    parcel.writeUInt8(psf.tileDataLow);
    parcel.writeUInt8(psf.tileDataHigh);

    IF_DEBUGGER(parcel.writeUInt64(cycles));
    IF_ASSERTS(parcel.writeBool(firstFetchWasBg));
}

void Ppu::loadState(Parcel& parcel) {
    static constexpr TickSelector TICK_SELECTORS[] {&Ppu::oamScan0,
                                                    &Ppu::oamScan1,
                                                    &Ppu::oamScanCompleted,
                                                    &Ppu::pixelTransferDummy0<false>,
                                                    &Ppu::pixelTransferDummy0<true>,
                                                    &Ppu::pixelTransferDiscard0,
                                                    &Ppu::pixelTransfer0,
                                                    &Ppu::pixelTransfer8,
                                                    &Ppu::hBlank,
                                                    &Ppu::vBlank};

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
    dots = parcel.readUInt16();
    LX = parcel.readUInt8();

    parcel.readBytes(bgFifo.data, sizeof(bgFifo.data));
    bgFifo.cursor = parcel.readUInt8();

    parcel.readBytes(objFifo.data, sizeof(objFifo.data));
    objFifo.start = parcel.readUInt8();
    objFifo.end = parcel.readUInt8();
    objFifo.count = parcel.readUInt8();

    parcel.readBytes(oamEntries, sizeof(oamEntries));
    IF_ASSERTS(oamEntriesCount = parcel.readUInt8());

    isFetchingSprite = parcel.readBool();

    oamScan.count = parcel.readUInt8();
    oamScan.entry.number = parcel.readUInt8();
    oamScan.entry.y = parcel.readUInt8();

    bgPixelTransfer.SCXmod8 = parcel.readUInt8();
    bgPixelTransfer.discardedPixels = parcel.readUInt8();

    bwf.LX = parcel.readUInt8();
    bwf.tilemapX = parcel.readUInt8();
    bwf.interruptedFetch.tileDataLow = parcel.readUInt8();
    bwf.interruptedFetch.tileDataHigh = parcel.readUInt8();
    bwf.interruptedFetch.hasData = parcel.readBool();

    w.WLY = parcel.readUInt8();
    w.shown = parcel.readBool();

    of.entry.number = parcel.readUInt8();
    of.entry.y = parcel.readUInt8();
    IF_ASSERTS(of.entry.x = parcel.readUInt8());
    of.tileNumber = parcel.readUInt8();
    of.attributes = parcel.readUInt8();

    psf.vTileDataAddress = parcel.readUInt16();
    psf.tileDataLow = parcel.readUInt8();
    psf.tileDataHigh = parcel.readUInt8();

    IF_DEBUGGER(cycles = parcel.readUInt64());
    IF_ASSERTS(firstFetchWasBg = parcel.readBool());
}
