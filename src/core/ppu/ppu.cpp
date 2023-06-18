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

static ILCD::Pixel fifo_pixel_to_lcd_pixel(const FIFO::Pixel &pixel) {
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

    if (on && !enabled) {
        on = false;
        lcd.turnOff();
        updateLY(0);
        fetcher.reset(); // TODO: bad...
        bgFifo.pixels.clear();
        objFifo.pixels.clear();
        dots = 0; // TODO: ?
        updateState(OAMScan); // TODO: here or on off -> on transition?
    } else if (!on && enabled) {
        on = true;
        lcd.turnOn();
    }

    if (!on)
        return; // TODO: ok?

    (this->*(tickHandlers[state]))();
    dots++;
    (this->*(afterTickHandlers[state]))();

    tCycles++;
}


void PPU::tick_HBlank() {
    // nop
}

void PPU::afterTick_HBlank() {
    if (dots == 456) {
        // end of hblank
        dots = 0;
        LX = 0;
        uint8_t LY = lcdIo.readLY();
        LY++;
        updateLY(LY);
        if (LY == 144) {
            // end of last hblank before vblank
            updateState(State::VBlank);
        } else {
            // start oam again
            updateState(State::OAMScan);
            scanlineOamEntries.clear(); // TODO: in updateState?
        }
    }
}



void PPU::tick_VBlank() {
    // nop
}

void PPU::afterTick_VBlank() {
    if (dots % 456 == 0) {
        uint8_t LY = lcdIo.readLY();
        LY++;
        if (LY < 154) {
            // end of current scanline, begin a new line but remain in vblank
            updateLY(LY);
        } else {
            // end of vblank
            dots = 0;
            updateState(State::OAMScan);
            scanlineOamEntries.clear(); // TODO: in updateState?
            updateLY(0);
        }
    }
}

void PPU::tick_OAMScan() {
    uint8_t LY = lcdIo.readLY();

    uint8_t oamNum = dots / 2;
    if (dots % 2 == 0) {
        // read oam entry y
        scratchpad.oamScan.y = oam.read(4 * oamNum);
    }
    else {
        uint8_t x = oam.read(4 * oamNum + 1);
        int oamY = scratchpad.oamScan.y - 16;
        if (oamY <= LY && LY < oamY + 8 )
            scanlineOamEntries.emplace_back(dots / 2, x, scratchpad.oamScan.y);
    }
}

void PPU::afterTick_OAMScan() {
    if (dots == 80) {
        // end of OAM scan
        LX = 0;
        fetcher.reset();
        updateState(PixelTransfer);
    }
}

void PPU::tick_PixelTransfer() {
    // shift pixel from fifo to lcd if bg fifo is not empty
    // and it's not blocked by the pixel fetcher due to sprite fetch

    scratchpad.pixelTransfer.pixelPushed = false;

    if (!isFifoBlocked()) {
        if (!bgFifo.pixels.empty()) {
            FIFO::Pixel bgPixel = bgFifo.pixels.front();
            bgFifo.pixels.pop_front();

            std::optional<FIFO::Pixel> objPixel;
            if (!objFifo.pixels.empty()) {
                objPixel = objFifo.pixels.front();
                objFifo.pixels.pop_front();
            }

            FIFO::Pixel pixel = bgPixel;

            // TODO: handle overlap
            if (objPixel)
                pixel = *objPixel;

            lcd.pushPixel(fifo_pixel_to_lcd_pixel(pixel));
            scratchpad.pixelTransfer.pixelPushed = true;
        }
    }

    fetcher.tick();
}

void PPU::afterTick_PixelTransfer() {
    if (scratchpad.pixelTransfer.pixelPushed) {
        LX++;

        if (LX >= 160) {
            // end of pixel transfer
            updateState(HBlank);
            // assert(dots >= 80 + 172 && dots <= 80 + 289);
        } else {
            // compute sprites on current LX
            std::vector<OAMEntry> entries;

            for (const auto &entry : scanlineOamEntries) {
                uint8_t oamX = entry.x - 8;
                if (oamX == LX)
                    entries.push_back(entry);
            }

            fetcher.setOAMEntriesHit(entries);
        }
    }
}


bool PPU::isFifoBlocked() const {
    return fetcher.isFetchingSprite();
}

void PPU::updateState(PPU::State s) {
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

void PPU::updateLY(uint8_t LY) {
 // write LY
    lcdIo.writeLY(LY);

    // update STAT's LYC=LY Flag
    uint8_t LYC = lcdIo.readLYC();
    uint8_t STAT = lcdIo.readSTAT();
    lcdIo.writeSTAT(set_bit<2>(STAT, LY == LYC));

    if ((get_bit<Bits::LCD::STAT::LYC_EQ_LY_INTERRUPT>(STAT)) && LY == LYC)
        interrupts.setIF<Bits::Interrupts::STAT>();
}


PPU::Fetcher::BGPrefetcher::BGPrefetcher(ILCDIO &lcdIo, IMemory &vram) :
    Processor<BGPrefetcher>(),
    lcdIo(lcdIo), vram(vram),
    tickHandlers {
        &PPU::Fetcher::BGPrefetcher::tick_GetTile1,
        &PPU::Fetcher::BGPrefetcher::tick_GetTile2,
    },
    x8(),
    tilemapX(),
    tilemapAddr(),
    tileNumber(),
    tileAddr(),
    tileDataAddr() {

}

void PPU::Fetcher::BGPrefetcher::tick_GetTile1() {
    uint8_t SCX = lcdIo.readSCX();
    tilemapX = (x8 + (SCX / 8)) % 32;
}

void PPU::Fetcher::BGPrefetcher::tick_GetTile2() {
    uint8_t SCY = lcdIo.readSCY();
    uint8_t LY = lcdIo.readLY(); // or fetcher.y?
    uint8_t tilemapY = ((LY + SCY) / 8) % 32;

    // fetch tile from tilemap
    uint16_t base = 0x9800;
    tilemapAddr = base + 32 * tilemapY + tilemapX;
    tileNumber = vram.read(tilemapAddr - MemoryMap::VRAM::START); // TODO: bad

    if (get_bit<Bits::LCD::LCDC::BG_WIN_TILE_DATA>(lcdIo.readLCDC()))
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
    }, tileNumber(),
    oamFlags(), tileAddr(),
    entry(), tileDataAddr() {

}

void PPU::Fetcher::OBJPrefetcher::tick_GetTile1() {
    tileNumber = oam.read(4 * entry.number + 2);
}

void PPU::Fetcher::OBJPrefetcher::tick_GetTile2() {
    oamFlags  = oam.read(4 * entry.number + 3);
    tileAddr = 0x8000 + tileNumber * 16 /* sizeof tile */;

    int oamY = entry.y - 16;
    int yOffset = lcdIo.readLY() - oamY;

    tileDataAddr = tileAddr + yOffset * 2;
}

bool PPU::Fetcher::OBJPrefetcher::isTileDataAddressReady() const {
    return dots == 2;
}

uint16_t PPU::Fetcher::OBJPrefetcher::getTileDataAddress() const {
    return tileDataAddr;
}

void PPU::Fetcher::OBJPrefetcher::setOAMEntry(const PPU::OAMEntry &oamEntry) {
    entry = oamEntry;
//    dots = 0;
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

PPU::Fetcher::Fetcher(ILCDIO &lcdIo, IMemory &vram, IMemory &oam, FIFO &bgFifo, FIFO &objFifo) :
    bgFifo(bgFifo), objFifo(objFifo),
    state(State::Prefetcher),
    targetFifo(FIFOType::Bg),
    bgPrefetcher(lcdIo, vram),
    objPrefetcher(lcdIo, oam),
    pixelSliceFetcher(vram) {

}

void PPU::Fetcher::reset() {
    state = State::Prefetcher;
    oamEntriesHit.clear();
    pixelSliceFetcher.reset();
    bgPrefetcher.reset();
    bgPrefetcher.resetTile();
    objPrefetcher.reset();
    bgFifo.pixels.clear();
    objFifo.pixels.clear();
}

void PPU::Fetcher::tick() {
    // TODO: bad here
    auto getPixelSlicerFetcherPixel = [&](uint8_t b) {
        uint8_t color =
                (get_bit(pixelSliceFetcher.getTileDataLow(), b) ? 0b01 : 0b00) |
                (get_bit(pixelSliceFetcher.getTileDataHigh(), b) ? 0b10 : 0b00);
        FIFO::Pixel p {
            .color = color
        };
        return p;
    };

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

            if (objPrefetcher.isTileDataAddressReady()) {
                objPrefetcher.reset();
                pixelSliceFetcher.setTileDataAddress(objPrefetcher.getTileDataAddress());
                pixelSliceFetcher.reset();
                state = State::PixelSliceFetcher;
            }
        }
    } else if (state == State::PixelSliceFetcher) {
        pixelSliceFetcher.tick();
        if (pixelSliceFetcher.isTileDataReady())
            state = State::Pushing;

        // TODO: oam read should overlap last bg push?
    } else if (state == State::Pushing) {
        if (targetFifo == FIFOType::Bg) {
            if (!oamEntriesHit.empty()) {
                // oam entry hit waiting to be served
                // discard bg push?
                objPrefetcher.setOAMEntry(pop(oamEntriesHit));
                state = State::Prefetcher;
                targetFifo = FIFOType::Obj;
            } else {
                if (bgFifo.pixels.empty()) {
                    // push pixels
                    for (int b = 7; b >= 0; b--)
                        bgFifo.pixels.push_back(getPixelSlicerFetcherPixel(b));

                    bgPrefetcher.advanceToNextTile();

                    state = State::Prefetcher;
                }
            }
        } else if (targetFifo == FIFOType::Obj) {
            objFifo.pixels.clear();

            // TODO: should not access flags so badly from
            bool backwardPush = true;
            if (get_bit<Bits::OAM::Attributes::X_FLIP>(objPrefetcher.oamFlags))
                backwardPush = false;

            // TODO: not clear/push but merge
            if (backwardPush) {
                for (int b = 7; b >= 0; b--)
                    objFifo.pixels.push_back(getPixelSlicerFetcherPixel(b));
            } else {
                for (int b = 0; b < 8; b++)
                    objFifo.pixels.push_back(getPixelSlicerFetcherPixel(b));
            }

            state = State::Prefetcher;

            if (!oamEntriesHit.empty()) {
                // oam entry hit waiting to be served
                // discard bg push?
                objPrefetcher.setOAMEntry(pop(oamEntriesHit));
                targetFifo = FIFOType::Obj;
            } else {
                targetFifo = FIFOType::Bg;
            }

        }
    }
}

void PPU::Fetcher::setOAMEntriesHit(const std::vector<OAMEntry> &entries) {
    oamEntriesHit = entries;
}

bool PPU::Fetcher::isFetchingSprite() const {
    return !oamEntriesHit.empty() || targetFifo == FIFOType::Obj;
}
