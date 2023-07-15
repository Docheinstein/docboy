#include <iostream>
#include "ppu.h"
#include "utils/binutils.h"
#include "core/definitions.h"
#include "core/memory/memory.h"
#include "core/lcd/lcd.h"
#include "core/io/interrupts.h"
#include <cassert>
#include <optional>
#include "utils/log.h"
#include "utils/stdutils.h"

using namespace Bits::LCD::LCDC;

static ILCD::Pixel fifo_pixel_to_lcd_pixel(const PPU::Pixel &pixel) {
    if (pixel.color == 0)
        return ILCD::Pixel::Color0;
    else if (pixel.color == 1)
        return ILCD::Pixel::Color1;
    else if (pixel.color == 2)
        return ILCD::Pixel::Color2;
    else if (pixel.color == 3)
        return ILCD::Pixel::Color3;
    throw std::runtime_error("unexpected color pixel: " + std::to_string(pixel.color));
}

PPU::PPU(ILCD &lcd, ILCDIO &lcdIo, IInterruptsIO &interrupts, IMemory &vram, IMemory &oam) :
    lcd(lcd), lcdIo(lcdIo), interrupts(interrupts),
    vram(vram), oam(oam),
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
    }
    else {
        // read oam entry x
        uint8_t x = oam.read(4 * oamNum + 1);
        // check if the sprite is upon this scanline
        int oamY = scratchpad.oamScan.y - 16;
        if (oamY <= LY() && LY() < oamY + 8)
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
        std::vector<OAMEntry> entries;

        for (const auto &entry : scanlineOamEntries) {
            if (entry.x < 8) {
                // TODO: handle this case
                continue;
            }
            if (entry.x == 8) {
                // TODO: handle this case
                continue;
            }
            uint8_t oamX = entry.x - 8;
            if (oamX == LX)
                entries.push_back(entry);
        }
        fetcher.setOAMEntriesHit(entries);
    };

    if (LX == 0 || scratchpad.pixelTransfer.pixelPushed)
        checkOamEntriesHit();

    scratchpad.pixelTransfer.pixelPushed = false;

    // TODO: figure out which is the right timing:
    //  does the fetcher tick happen before pushing the pixel to the LCD?
    fetcher.tick();

    if (!isFifoBlocked()) {
        if (!bgFifo.empty()) {
            Pixel bgPixel = bgFifo.front();
            bgFifo.pop_front();

            std::optional<Pixel> objPixel;
            if (!objFifo.empty()) {
                objPixel = objFifo.front();
                objFifo.pop_front();
            }

            Pixel pixel = bgPixel;

            if (objPixel && objPixel->color)
                pixel = *objPixel;

            lcd.pushPixel(fifo_pixel_to_lcd_pixel(pixel));
            scratchpad.pixelTransfer.pixelPushed = true;
        }
    }
}

void PPU::afterTick_PixelTransfer() {
    if (scratchpad.pixelTransfer.pixelPushed) {
        LX++;
        assert(LX <= 160);
        if (LX == 160)
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
    static constexpr uint8_t DOTS_ERROR_THRESHOLD_TO_FIX = 4;
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
}

void PPU::turnOff() {
    assert(on == true);
    on = false;
    lcd.turnOff();
    fetcher.reset();
    setLY(0);
    enterOAMScan();
}


PPU::Fetcher::BGPrefetcher::BGPrefetcher(ILCDIO &lcdIo, IMemory &vram) :
    Processor<BGPrefetcher>(),
    lcdIo(lcdIo), vram(vram),
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
    tileDataAddr() {

}

void PPU::Fetcher::BGPrefetcher::tick_GetTile1() {
    uint8_t WX = lcdIo.readWX();
    uint8_t WY = lcdIo.readWY();
    uint8_t LY = lcdIo.readLY();

    fetchType = FetchType::Background;

    if (get_bit<Bits::LCD::LCDC::WIN_ENABLE>(lcdIo.readLCDC())) {
        // assert((WX - 7) % 8 == 0);
        // TODO: figure out how to handle WX - 7 not dividing 8
        if ((WX - 7) % 8 == 0) { // TODO: remove this check
            if ((8 * x8 >= WX - 7 && LY >= WY))
                fetchType = FetchType::Window;
        }
    }

    if (fetchType == FetchType::Background) {
        uint8_t SCX = lcdIo.readSCX();
        tilemapX = (x8 + (SCX / 8)) % 32;
    } else /* Window */ {
        assert((8 * x8 - (WX - 7)) % 8 == 0);
        tilemapX = (8 * x8 - (WX - 7)) / 8;
    }
}

void PPU::Fetcher::BGPrefetcher::tick_GetTile2() {
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
    }  else /* Window */{
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

    uint16_t tileY = (LY + SCY) % 8;
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
    x8 = (x8 + 1) % 20;
}

PPU::Fetcher::OBJPrefetcher::OBJPrefetcher(ILCDIO &lcdIo, IMemory &oam) :
    Processor<OBJPrefetcher>(),
    lcdIo(lcdIo), oam(oam),
    tickHandlers {
        &PPU::Fetcher::OBJPrefetcher::tick_GetTile1,
        &PPU::Fetcher::OBJPrefetcher::tick_GetTile2,
    },
    tileNumber(),
    entry(),
    tileDataAddr(),
    oamFlags() {

}

void PPU::Fetcher::OBJPrefetcher::tick_GetTile1() {
    tileNumber = oam.read(4 * entry.number + 2);
}

void PPU::Fetcher::OBJPrefetcher::tick_GetTile2() {
    oamFlags  = oam.read(4 * entry.number + 3);
    uint16_t tileAddr = 0x8000 + tileNumber * 16 /* sizeof tile */;

    int oamY = entry.y - 16;
    int yOffset = lcdIo.readLY() - oamY;

    tileDataAddr = tileAddr + yOffset * 2;
}

bool PPU::Fetcher::OBJPrefetcher::areTileDataAddressAndFlagsReady() const {
    return dots == 2;
}

uint16_t PPU::Fetcher::OBJPrefetcher::getTileDataAddress() const {
    return tileDataAddr;
}

void PPU::Fetcher::OBJPrefetcher::setOAMEntry(const PPU::OAMEntry &oamEntry) {
    entry = oamEntry;
}

PPU::OAMEntry PPU::Fetcher::OBJPrefetcher::getOAMEntry() const {
    return entry;
}

uint8_t PPU::Fetcher::OBJPrefetcher::getOAMFlags() const {
    return oamFlags;
}

PPU::Fetcher::PixelSliceFetcher::PixelSliceFetcher(IMemory &vram) :
    Processor(),
    vram(vram),
    tickHandlers {
        &PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataLow_1,
        &PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataLow_2,
        &PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataHigh_1,
        &PPU::Fetcher::PixelSliceFetcher::tick_GetTileDataHigh_2,
    }, tileDataAddr(), tileDataLow(), tileDataHigh() {
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

PPU::Fetcher::Fetcher(ILCDIO &lcdIo, IMemory &vram, IMemory &oam, PixelFIFO &bgFifo, PixelFIFO &objFifo) :
    bgFifo(bgFifo), objFifo(objFifo),
    state(State::Prefetcher),
    targetFifo(FIFOType::Bg),
    firstFetch(true),
    bgPrefetcher(lcdIo, vram),
    objPrefetcher(lcdIo, oam),
    pixelSliceFetcher(vram),
    dots() {

}

void PPU::Fetcher::reset() {
    state = State::Prefetcher;
    firstFetch = true;
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

    auto emplacePixelSliceFetcherData = [&](auto &pixels, bool flip = false) {
        uint8_t low = pixelSliceFetcher.getTileDataLow();
        uint8_t high = pixelSliceFetcher.getTileDataHigh();

        if (flip) {
            for (int b = 0; b < 8; b++)
                pixels.emplace_back((get_bit(low, b) ? 0b01 : 0b00) | (get_bit(high, b) ? 0b10 : 0b00));
        } else {
            for (int b = 7; b >= 0; b--)
                pixels.emplace_back((get_bit(low, b) ? 0b01 : 0b00) | (get_bit(high, b) ? 0b10 : 0b00));
        }
    };

    auto emplacePixelSliceFetcherDataForOAMEntry = [&](auto &pixels, const OAMEntry &entry, bool flip = false) {
        assert(pixels.empty());
        emplacePixelSliceFetcherData(pixels, flip);
        for (auto &pixel : pixels) {
            pixel.priority = entry.number;
            pixel.x = entry.x;
        }
    };


    auto restartFetcher = [&]{
        state = State::Prefetcher;
        dots = 0;
    };

    // TODO: quite bad looking
    auto restartFetcherWithOAMEntryAndTick = [this, restartFetcher](const OAMEntry &entry) {
        // oam entry hit waiting to be served
        // discard bg push?
        objPrefetcher.setOAMEntry(entry);
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
                    // push pixels
                    // TODO: first fetch is a dummy fetch, figure out how to handle this properly
                    if (!firstFetch) {
                        emplacePixelSliceFetcherData(bgFifo);
                        bgPrefetcher.advanceToNextTile();
                    }
                    firstFetch = false;
                    restartFetcher();
                }
            }
        } else if (targetFifo == FIFOType::Obj) {
            std::vector<Pixel> sprite;

            // TODO: should not access flags so badly from here
            bool flip = get_bit<Bits::OAM::Attributes::X_FLIP>(objPrefetcher.getOAMFlags());

            // retrieve sprite's pixels
#if 1
            emplacePixelSliceFetcherDataForOAMEntry(sprite, objPrefetcher.getOAMEntry(), flip);
            // handle sprite-to-sprite conflicts by merging sprite with objFifo
            // "The smaller the X coordinate, the higher the priority."
            // "When X coordinates are identical, the object located first in OAM has higher priority."

            uint8_t i = 0;
            uint8_t objFifoSize = objFifo.size();
            while (i < objFifoSize) {
                // handle conflict
                const Pixel &spritePixel = sprite[i];
                const Pixel &fifoPixel = objFifo[i];
                // Overwrite pixel in fifo with pixel in sprite if:
                // 1. Pixel in sprite is opaque and pixel in fifo is transparent
                // 2. Both pixels in sprite and fifo are opaque but pixel in sprite
                //    has either lower x or, if x is equal, lowest oam number.
                if (
                    (spritePixel.color && !fifoPixel.color)
                        ||
                    ((spritePixel.color && fifoPixel.color) &&
                        ((spritePixel.x < fifoPixel.x) ||
                        ((spritePixel.x == fifoPixel.x) && (spritePixel.priority < fifoPixel.priority))))

                    ) {
                    objFifo[i] = spritePixel;
                }
                i++;
            }

            while (i < sprite.size())
                objFifo.push_back(sprite[i++]);
#else
            objFifo.clear();
            emplacePixelSliceFetcherData(objFifo, flip);
#endif
            if (!oamEntriesHit.empty()) {
                // oam entry hit waiting to be served
                // discard bg push?
                objPrefetcher.setOAMEntry(pop(oamEntriesHit));
                targetFifo = FIFOType::Obj;
            } else {
                targetFifo = FIFOType::Bg;
            }

            restartFetcher();
        }
    }
}

void PPU::Fetcher::setOAMEntriesHit(const std::vector<OAMEntry> &entries) {
    assert(entries.size() <= 10);
    oamEntriesHit = entries;
}

bool PPU::Fetcher::isFetchingSprite() const {
    return !oamEntriesHit.empty() || targetFifo == FIFOType::Obj;
}
