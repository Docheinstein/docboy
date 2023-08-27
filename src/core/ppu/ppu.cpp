#include "ppu.h"
#include "core/definitions.h"
#include "core/io/interrupts.h"
#include "core/lcd/lcd.h"
#include "core/memory/memory.h"
#include "utils/binutils.h"
#include "utils/log.h"
#include "utils/stdutils.h"
#include <array>
#include <cassert>
#include <iostream>
#include <optional>

using namespace Bits::LCD::LCDC;

static std::array<ILCD::Pixel, 4> COLOR_TO_LCD_PIXEL = {
    ILCD::Pixel::Color0,
    ILCD::Pixel::Color1,
    ILCD::Pixel::Color2,
    ILCD::Pixel::Color3,
};

static ILCD::Pixel color_to_lcd_pixel(uint8_t color) {
    assert(color < 4);
    return COLOR_TO_LCD_PIXEL[color];
}

PPU::OAMEntry::OAMEntry(const PPU::OAMEntryFetchInfo& other) :
    OAMEntryFetchInfo(other) {
}

PPU::Pixel::Pixel(uint8_t colorIndex) :
    colorIndex(colorIndex) {
}

PPU::OBJPixel::OBJPixel(uint8_t colorIndex, const PPU::OAMEntry& entry) :
    Pixel(colorIndex) {
    palette = get_bit<Bits::OAM::Attributes::PALETTE_NUM>(entry.flags);
    priority = entry.number;
    x = entry.x;
    bgOverObj = get_bit<Bits::OAM::Attributes::BG_OVER_OBJ>(entry.flags);
}

PPU::PPU(ILCD& lcd, ILCDIO& lcdIo, IInterruptsIO& interrupts, IMemory& vram, IMemory& oam) :
    lcd(lcd),
    lcdIo(lcdIo),
    interrupts(interrupts),
    vram(vram),
    oam(oam),
    tickHandlers {
        &PPU::tick_HBlank,
        &PPU::tick_VBlank,
        &PPU::tick_OAMScan,
        &PPU::tick_PixelTransfer,
    },
    afterTickHandlers {
        &PPU::afterTick_HBlank,
        &PPU::afterTick_VBlank,
        &PPU::afterTick_OAMScan,
        &PPU::afterTick_PixelTransfer,
    },
    on(),
    state(OAMScan),
    fetcher(lcdIo, vram, oam, bgFifo, objFifo),
    bgFifo(),
    objFifo(),
    LX(),
    dots(),
    tCycles() {
    fetcher.reset();
}

/*
 * TODO
Writing a value of 0 to bit 7 of the LCDC register when its value is 1 stops the LCD controller, and
the value of register LY immediately becomes 0. (Note: Values should not be written to the
register during screen display.)
 */

void PPU::tick() {
    bool enabled = get_bit<LCD_ENABLE>(lcdIo.readLCDC());

    if (on && !enabled)
        turnOff();
    else if (!on && enabled)
        turnOn();

    if (!on)
        return; // TODO: ok?

    (this->*(tickHandlers[state]))();
    dots++;
    (this->*(afterTickHandlers[state]))();

    assert((state == VBlank) ? (dots >= 0 && dots < 4560) : (dots >= 0 && dots < 456));

    tCycles++;
}

void PPU::loadState(IReadableState& gameState) {
    // TODO
}

void PPU::saveState(IWritableState& gameState) {
    // TODO
}

void PPU::tick_HBlank() {
    // nop
}

void PPU::afterTick_HBlank() {
    if (dots == 456) {
        setLY(LY() + 1);
        if (LY() == 144)
            enterVBlank();
        else
            enterOAMScan();
    }
}

void PPU::tick_VBlank() {
    // nop
}

void PPU::afterTick_VBlank() {
    if (dots % 456 == 0) {
        setLY((LY() + 1) % 154);
        if (LY() == 0)
            // end of vblank
            enterOAMScan();
    }
}

void PPU::tick_OAMScan() {
    if (scanlineOamEntries.size() >= 10)
        // the maximum number of oam entries per scanline is 10
        return;

    uint8_t oamNum = dots / 2;
    if (dots % 2 == 0) {
        // read oam entry y
        scratchpad.oamScan.y = oam.read(4 * oamNum);
    } else {
        // read oam entry height
        uint8_t objHeight = get_bit<Bits::LCD::LCDC::OBJ_SIZE>(lcdIo.readLCDC()) ? 16 : 8;

        // read oam entry x
        uint8_t x = oam.read(4 * oamNum + 1);

        // check if the sprite is upon this scanline
        int oamY = scratchpad.oamScan.y - 16;
        if (oamY <= LY() && LY() < oamY + objHeight)
            scanlineOamEntries.emplace_back(dots / 2, x, scratchpad.oamScan.y);
    }

    assert(scanlineOamEntries.size() <= 10);
}

void PPU::afterTick_OAMScan() {
    if (dots == 80)
        enterPixelTransfer();
}

void PPU::tick_PixelTransfer() {
    // shift pixel from fifo to lcd if bg fifo is not empty
    // and it's not blocked by the pixel fetcher due to sprite fetch

    auto checkOamEntriesHit = [&]() {
        // compute sprites on current LX

        fetcher.clearOAMEntriesHit();

        for (const auto& entry : scanlineOamEntries) {
            if (entry.x == LX)
                fetcher.addOAMEntryHit(entry);
        }
    };

    if (scratchpad.pixelTransfer.firstOamEntriesCheck || scratchpad.pixelTransfer.pixelPushed) {
        scratchpad.pixelTransfer.firstOamEntriesCheck = false;
        checkOamEntriesHit();
    }

    scratchpad.pixelTransfer.pixelPushed = false;

    // TODO: figure out which is the right timing:
    //  does the fetcher tick happen before pushing the pixel to the LCD?
    fetcher.tick();

    if (!isFifoBlocked()) {
        if (!bgFifo.empty()) {
            // pop out a pixel from BG fifo
            BGPixel bgPixel = pop_front(bgFifo);

            // eventually pop out a pixel from OBJ fifo
            std::optional<OBJPixel> objPixel;
            if (!objFifo.empty()) {
                objPixel = pop_front(objFifo);
            }

            uint8_t colorIndex;
            uint8_t palette;

            // handle overlap between BG and OBJ pixel

            /*
             * BG_OVER_OBJ
             * (0=OBJ Above BG, 1=OBJ Behind BG color 1-3, BG color 0 is always behind OBJ)
             */

            if (objPixel /* take pixel from obj only if exists */ &&
                objPixel->colorIndex != 0 /* and it's not transparent */ &&
                (objPixel->bgOverObj == 0 ||
                 bgPixel.colorIndex == 0) /* and BG_OVER_OBJ is disabled or the BG color is not within [1-3] */) {
                colorIndex = objPixel->colorIndex;
                palette = objPixel->palette == 0 ? lcdIo.readOBP0() : lcdIo.readOBP1();
            } else {
                colorIndex = bgPixel.colorIndex;
                palette = lcdIo.readBGP();
            }

            // resolve real color using palette and the index of the color
            // ColorIndex |  3  |  2  |  1  |  0  |
            // PaletteBit | 7 6 | 5 4 | 3 2 | 1 0 |
            // Color      | ? ? | ? ? | ? ? | ? ? |
            uint8_t color = keep_bits<2>(palette >> (2 * colorIndex));

            if (LX >= 8 /* first 8 pixels are discarded */) {
                lcd.pushPixel(color_to_lcd_pixel(color));
            }

            scratchpad.pixelTransfer.pixelPushed = true;
        }
    }
}

void PPU::afterTick_PixelTransfer() {
    if (scratchpad.pixelTransfer.pixelPushed) {
        LX++;
        assert(LX <= 168);
        if (LX == 168)
            enterHBlank();
    }
}

bool PPU::isFifoBlocked() const {
    return fetcher.isFetchingSprite();
}

void PPU::enterOAMScan() {
    assert(state == HBlank || state == VBlank);
    assert(LY() < 144);
    dots = 0;
    scanlineOamEntries.clear();
    setState(OAMScan);
}

void PPU::enterHBlank() {
    assert(state == PixelTransfer);
    assert(LY() < 144);
    // TODO: does not pass right now: timing of PixelTransfer is slightly wrong?
    static constexpr uint8_t DOTS_ERROR_THRESHOLD_TO_FIX = 16;
    assert(dots >= 80 + 172 && dots <= 80 + 289 + DOTS_ERROR_THRESHOLD_TO_FIX);
    setState(HBlank);
}

void PPU::enterVBlank() {
    assert(state == HBlank);
    assert(LY() == 144);
    dots = 0;
    setState(VBlank);
}

void PPU::enterPixelTransfer() {
    assert(state == OAMScan);
    assert(LY() < 144);
    assert(dots == 80);
    LX = 0;
    fetcher.reset();
    scratchpad.pixelTransfer.firstOamEntriesCheck = true; // TODO: refactor: some kind of static state pattern?
    setState(PixelTransfer);
}

void PPU::setState(PPU::State s) {
    // update STAT's Mode Flag
    state = s;
    uint8_t STAT = lcdIo.readSTAT();
    STAT = reset_bits<2>(STAT) | state;
    lcdIo.writeSTAT(STAT);

    // eventually raise STAT interrupt
    if ((get_bit<Bits::LCD::STAT::OAM_INTERRUPT>(STAT) && state == OAMScan) ||
        (get_bit<Bits::LCD::STAT::VBLANK_INTERRUPT>(STAT) && state == VBlank) ||
        (get_bit<Bits::LCD::STAT::HBLANK_INTERRUPT>(STAT) && state == HBlank)) {
        interrupts.setIF<Bits::Interrupts::STAT>();
    }

    // eventually raise
    if (state == VBlank)
        interrupts.setIF<Bits::Interrupts::VBLANK>();
}

void PPU::setLY(uint8_t LY) {
    // write LY
    lcdIo.writeLY(LY);

    // update STAT's LYC=LY Flag
    uint8_t LYC = lcdIo.readLYC();
    uint8_t STAT = lcdIo.readSTAT();
    lcdIo.writeSTAT(set_bit<2>(STAT, LY == LYC));

    if ((get_bit<Bits::LCD::STAT::LYC_EQ_LY_INTERRUPT>(STAT)) && LY == LYC)
        interrupts.setIF<Bits::Interrupts::STAT>();
}

uint8_t PPU::LY() const {
    return lcdIo.readLY();
}

void PPU::turnOn() {
    assert(on == false);
    on = true;
    lcd.turnOn();

    // TODO: makes STAT and state consistent, but is probably wrong: figure out the internals
    lcdIo.writeSTAT(reset_bits<2>(lcdIo.readSTAT()) | state);
}

void PPU::turnOff() {
    assert(on == true);
    on = false;
    lcd.turnOff();
    fetcher.reset();
    setLY(0);
    enterOAMScan();

    // Apparently lasts two bits of STAT should be 0 when PPU is turned off
    // TODO: figure out what is actually reset here
    lcdIo.writeSTAT(reset_bits<2>(lcdIo.readSTAT()));
}

PPU::Fetcher::BGPrefetcher::BGPrefetcher(ILCDIO& lcdIo, IMemory& vram) :
    Processor<BGPrefetcher>(),
    lcdIo(lcdIo),
    vram(vram),
    tickHandlers {
        &PPU::Fetcher::BGPrefetcher::tick_GetTile1,
        &PPU::Fetcher::BGPrefetcher::tick_GetTile2,
    },
    x8(),
    fetchType(),
    tilemapX(),
    tilemapAddr(),
    tileNumber(),
    tileAddr(),
    pixelsToDiscard(), // TODO: refactor this
    tileDataAddr() {
}

void PPU::Fetcher::BGPrefetcher::tick_GetTile1() {
    uint8_t WX = lcdIo.readWX();
    uint8_t WY = lcdIo.readWY();
    uint8_t LY = lcdIo.readLY();
    uint8_t LX_ = 8 * x8;

    fetchType = FetchType::Background;

    if (get_bit<Bits::LCD::LCDC::WIN_ENABLE>(lcdIo.readLCDC())) {
        // assert((WX - 7) % 8 == 0);
        // TODO: figure out how to handle WX - 7 not dividing 8
        if ((WX - 7) % 8 == 0) { // TODO: remove this check
            if ((LX_ >= WX - 7 && LY >= WY))
                fetchType = FetchType::Window;
        }
    }

    if (fetchType == FetchType::Background) {
        uint8_t SCX = lcdIo.readSCX();
        tilemapX = ((LX_ + SCX) / 8) % 32;
        pixelsToDiscard = x8 == 0 ? SCX % 8 : 0;
    } else /* Window */ {
        assert((LX_ - (WX - 7)) % 8 == 0);
        tilemapX = (LX_ - (WX - 7)) / 8;
        pixelsToDiscard = 0;
    }
}

void PPU::Fetcher::BGPrefetcher::tick_GetTile2() {
    // TODO: refactor this: split BG and window

    uint8_t SCY = lcdIo.readSCY();
    uint8_t LY = lcdIo.readLY(); // or fetcher.y?

    uint8_t tilemapY;
    if (fetchType == FetchType::Background) {
        tilemapY = ((LY + SCY) / 8) % 32;
    } else /* Window */ {
        uint8_t WY = lcdIo.readWY();
        assert(((LY - WY) / 8) < 32);
        tilemapY = (LY - WY) / 8;
    }

    uint8_t LCDC = lcdIo.readLCDC();

    // fetch tile number from tilemap
    uint16_t tilemapBase;
    if (fetchType == FetchType::Background) {
        tilemapBase = get_bit<Bits::LCD::LCDC::BG_TILE_MAP>(LCDC) ? 0x9C00 : 0x9800;
    } else /* Window */ {
        tilemapBase = get_bit<Bits::LCD::LCDC::WIN_TILE_MAP>(LCDC) ? 0x9C00 : 0x9800;
    }

    tilemapAddr = tilemapBase + 32 * tilemapY + tilemapX;
    tileNumber = vram.read(tilemapAddr - MemoryMap::VRAM::START); // TODO: bad

    if (get_bit<Bits::LCD::LCDC::BG_WIN_TILE_DATA>(LCDC))
        tileAddr = 0x8000 + tileNumber * 16 /* sizeof tile */;
    else {
        if (tileNumber < 128U)
            tileAddr = 0x9000 + tileNumber * 16 /* sizeof tile */;
        else
            tileAddr = 0x8800 + (tileNumber - 128U) * 16 /* sizeof tile */;
    }

    uint16_t tileY;

    if (fetchType == FetchType::Background) {
        tileY = (LY + SCY) % 8;
    } else /* Window */ {
        uint8_t WY = lcdIo.readWY();
        tileY = (LY - WY) % 8;
    }

    tileDataAddr = tileAddr + tileY * 2 /* sizeof tile row */;
}

uint16_t PPU::Fetcher::BGPrefetcher::getTileDataAddress() const {
    return tileDataAddr;
}

bool PPU::Fetcher::BGPrefetcher::isTileDataAddressReady() const {
    return dots == 2;
}

void PPU::Fetcher::BGPrefetcher::resetTile() {
    x8 = 0;
}

void PPU::Fetcher::BGPrefetcher::advanceToNextTile() {
    x8 = (x8 + 1) % 32;
}

uint8_t PPU::Fetcher::BGPrefetcher::getPixelsToDiscardCount() const {
    return pixelsToDiscard;
}

PPU::Fetcher::OBJPrefetcher::OBJPrefetcher(ILCDIO& lcdIo, IMemory& oam) :
    Processor<OBJPrefetcher>(),
    lcdIo(lcdIo),
    oam(oam),
    tickHandlers {
        &PPU::Fetcher::OBJPrefetcher::tick_GetTile1,
        &PPU::Fetcher::OBJPrefetcher::tick_GetTile2,
    },
    tileNumber(),
    tileDataAddr(),
    entry() {
}

void PPU::Fetcher::OBJPrefetcher::tick_GetTile1() {
    tileNumber = oam.read(4 * entry.number + 2);
}

void PPU::Fetcher::OBJPrefetcher::tick_GetTile2() {
    entry.flags = oam.read(4 * entry.number + 3);
    uint16_t tileAddr = 0x8000 + tileNumber * 16 /* sizeof tile */;

    int oamY = entry.y - 16;
    int yOffset = lcdIo.readLY() - oamY;

    assert(yOffset >= 0 && yOffset < (get_bit<Bits::LCD::LCDC::OBJ_SIZE>(lcdIo.readLCDC()) ? 16 : 8));

    if (get_bit<Bits::OAM::Attributes::Y_FLIP>(entry.flags)) {
        uint8_t objHeight = get_bit<Bits::LCD::LCDC::OBJ_SIZE>(lcdIo.readLCDC()) ? 16 : 8;
        yOffset = objHeight - yOffset;
    }

    tileDataAddr = tileAddr + yOffset * 2 /* sizeof tile row */; // works both for 8x8 and 8x16 OBJ
}

bool PPU::Fetcher::OBJPrefetcher::areTileDataAddressAndFlagsReady() const {
    return dots == 2;
}

uint16_t PPU::Fetcher::OBJPrefetcher::getTileDataAddress() const {
    return tileDataAddr;
}

void PPU::Fetcher::OBJPrefetcher::setOAMEntryFetchInfo(const PPU::OAMEntryFetchInfo& oamEntry) {
    entry = oamEntry;
}

PPU::OAMEntry PPU::Fetcher::OBJPrefetcher::getOAMEntry() const {
    return entry;
}

PPU::Fetcher::PixelSliceFetcher::PixelSliceFetcher(IMemory& vram) :
    Processor(),
    vram(vram),
    tickHandlers {
        &PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataLow_1,
        &PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataLow_2,
        &PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataHigh_1,
        &PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataHigh_2,
    },
    tileDataAddr(),
    tileDataLow(),
    tileDataHigh() {
}

void PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataLow_1() {
    tileDataLow = vram.read(tileDataAddr - MemoryMap::VRAM::START);
}

void PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataLow_2() {
}

void PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataHigh_1() {
    tileDataHigh = vram.read(tileDataAddr - MemoryMap::VRAM::START + 1);
}

void PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataHigh_2() {
}

void PPU::Fetcher::PixelSliceFetcher::setTileDataAddress(uint16_t tileDataAddr_) {
    tileDataAddr = tileDataAddr_;
    //    dots = 0;
}

bool PPU::Fetcher::PixelSliceFetcher::isTileDataReady() const {
    return dots == 4;
}

uint8_t PPU::Fetcher::PixelSliceFetcher::getTileDataLow() const {
    return tileDataLow;
}

uint8_t PPU::Fetcher::PixelSliceFetcher::getTileDataHigh() const {
    return tileDataHigh;
}

PPU::Fetcher::Fetcher(ILCDIO& lcdIo, IMemory& vram, IMemory& oam, BGPixelFIFO& bgFifo, OBJPixelFIFO& objFifo) :
    bgFifo(bgFifo),
    objFifo(objFifo),
    state(State::Prefetcher),
    targetFifo(FIFOType::Bg),
    fetchNumber(0),
    bgPrefetcher(lcdIo, vram),
    objPrefetcher(lcdIo, oam),
    pixelSliceFetcher(vram),
    dots() {
}

void PPU::Fetcher::reset() {
    state = State::Prefetcher;
    fetchNumber = 0;
    oamEntriesHit.clear();
    pixelSliceFetcher.reset();
    bgPrefetcher.reset();
    bgPrefetcher.resetTile();
    objPrefetcher.reset();
    bgFifo.clear();
    objFifo.clear();
    dots = 0;
}

void PPU::Fetcher::tick() {
    // TODO: bad here

    auto emplacePixelSliceFetcherDataFromBG = [&](auto& pixels) {
        uint8_t low = pixelSliceFetcher.getTileDataLow();
        uint8_t high = pixelSliceFetcher.getTileDataHigh();

        for (int b = 7; b >= 0; b--) {
            pixels.emplace_back((get_bit(low, b) ? 0b01 : 0b00) | (get_bit(high, b) ? 0b10 : 0b00));
        }
    };

    auto emplacePixelSliceFetcherDataFromOBJ = [&](auto& pixels, const OAMEntry& entry) {
        assert(pixels.empty());

        uint8_t low = pixelSliceFetcher.getTileDataLow();
        uint8_t high = pixelSliceFetcher.getTileDataHigh();

        bool xFlip = get_bit<Bits::OAM::Attributes::X_FLIP>(entry.flags);

        if (xFlip) {
            for (int b = 0; b < 8; b++)
                pixels.emplace_back((get_bit(low, b) ? 0b01 : 0b00) | (get_bit(high, b) ? 0b10 : 0b00), entry);
        } else {
            for (int b = 7; b >= 0; b--)
                pixels.emplace_back((get_bit(low, b) ? 0b01 : 0b00) | (get_bit(high, b) ? 0b10 : 0b00), entry);
        }
    };

    auto restartFetcher = [&] {
        state = State::Prefetcher;
        dots = 0;
    };

    // TODO: quite bad looking
    auto restartFetcherWithOAMEntryAndTick = [this, restartFetcher](const OAMEntryFetchInfo& entry) {
        // oam entry hit waiting to be served
        // discard bg push?
        objPrefetcher.setOAMEntryFetchInfo(entry);
        targetFifo = FIFOType::Obj;
        restartFetcher();

        objPrefetcher.tick();
        assert(!objPrefetcher.areTileDataAddressAndFlagsReady());
    };

    dots++;

    if (state == State::Prefetcher) {
        if (targetFifo == FIFOType::Bg) {
            bgPrefetcher.tick();

            if (bgPrefetcher.isTileDataAddressReady()) {
                bgPrefetcher.reset();
                pixelSliceFetcher.setTileDataAddress(bgPrefetcher.getTileDataAddress());
                pixelSliceFetcher.reset();
                state = State::PixelSliceFetcher;
            }
        } else if (targetFifo == FIFOType::Obj) {
            objPrefetcher.tick();

            if (objPrefetcher.areTileDataAddressAndFlagsReady()) {
                objPrefetcher.reset();
                pixelSliceFetcher.setTileDataAddress(objPrefetcher.getTileDataAddress());
                pixelSliceFetcher.reset();
                state = State::PixelSliceFetcher;
            }
        }
    } else if (state == State::PixelSliceFetcher) {
        pixelSliceFetcher.tick();
        if (pixelSliceFetcher.isTileDataReady()) {
            state = State::Pushing;

            // first oam read should overlap last bg push
            // TODO: check if this is right
            if (targetFifo == FIFOType::Bg) {
                if (!oamEntriesHit.empty()) {
                    restartFetcherWithOAMEntryAndTick(pop(oamEntriesHit));
                }
            }
        }
    } else if (state == State::Pushing) {
        if (targetFifo == FIFOType::Bg) {
            if (!oamEntriesHit.empty()) {
                restartFetcherWithOAMEntryAndTick(pop(oamEntriesHit));
            } else {
                if (bgFifo.empty()) {
                    emplacePixelSliceFetcherDataFromBG(bgFifo);
                    if (fetchNumber > 0) {
                        bgPrefetcher.advanceToNextTile();

                        // TODO: bad design
                        for (int i = 0; i < bgPrefetcher.getPixelsToDiscardCount(); i++) {
                            pop_front(bgFifo);
                        }
                    }
                    fetchNumber++;
                    restartFetcher();
                }
            }
        } else if (targetFifo == FIFOType::Obj) {
            std::vector<OBJPixel> sprite;

            // TODO: should not access flags so badly from here, put flags into oam entry?
            // retrieve sprite's pixels

            emplacePixelSliceFetcherDataFromOBJ(sprite, objPrefetcher.getOAMEntry());
            // handle sprite-to-sprite conflicts by merging sprite with objFifo
            // "The smaller the X coordinate, the higher the priority."
            // "When X coordinates are identical, the object located first in OAM has higher priority."

            uint8_t i = 0;
            uint8_t objFifoSize = objFifo.size();
            while (i < objFifoSize) {
                // handle conflict
                const OBJPixel& spritePixel = sprite[i];
                const OBJPixel& fifoPixel = objFifo[i];
                // Overwrite pixel in fifo with pixel in sprite if:
                // 1. Pixel in sprite is opaque and pixel in fifo is transparent
                // 2. Both pixels in sprite and fifo are opaque but pixel in sprite
                //    has either lower x or, if x is equal, lowest oam number.
                if ((spritePixel.colorIndex && !fifoPixel.colorIndex) ||
                    ((spritePixel.colorIndex && fifoPixel.colorIndex) &&
                     ((spritePixel.x < fifoPixel.x) ||
                      ((spritePixel.x == fifoPixel.x) && (spritePixel.priority < fifoPixel.priority))))

                ) {
                    objFifo[i] = spritePixel;
                }
                i++;
            }

            while (i < sprite.size())
                objFifo.push_back(sprite[i++]);

            if (!oamEntriesHit.empty()) {
                // oam entry hit waiting to be served
                // discard bg push?
                objPrefetcher.setOAMEntryFetchInfo(pop(oamEntriesHit));
                targetFifo = FIFOType::Obj;
            } else {
                targetFifo = FIFOType::Bg;
            }

            restartFetcher();
        }
    }
}

bool PPU::Fetcher::isFetchingSprite() const {
    return !oamEntriesHit.empty() || targetFifo == FIFOType::Obj;
}

void PPU::Fetcher::addOAMEntryHit(const PPU::OAMEntryFetchInfo& entry) {
    oamEntriesHit.push_back(entry);
    assert(oamEntriesHit.size() <= 10);
}

void PPU::Fetcher::clearOAMEntriesHit() {
    oamEntriesHit.clear();
}
