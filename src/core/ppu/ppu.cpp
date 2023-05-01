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
    on(),
    state(OAMScan),
    bgFifo(),
    objFifo(),
    LX(), fetcher(),
    dots(),
    tCycles() {
    fetcherClear();
}

/*
 * TODO
Writing a value of 0 to bit 7 of the LCDC register when its value is 1 stops the LCD controller, and
the value of register LY immediately becomes 0. (Note: Values should not be written to the
register during screen display.)
 */

void PPU::tick() {
    auto updateState = [&](State s) {
        // update STAT's Mode Flag
        state = s;
        uint8_t STAT = lcdIo.readSTAT();
        STAT = reset_bits<2>(STAT) | s;
        lcdIo.writeSTAT(STAT);

        // eventually raise STAT interrupt
        if ((get_bit<Bits::LCD::STAT::OAM_INTERRUPT>(STAT) && s == OAMScan) ||
                (get_bit<Bits::LCD::STAT::VBLANK_INTERRUPT>(STAT) && s == VBlank) ||
                (get_bit<Bits::LCD::STAT::HBLANK_INTERRUPT>(STAT) && s == HBlank)) {
            interrupts.setIF<Bits::Interrupts::STAT>();
        }

        // eventually raise
        if (s == VBlank)
            interrupts.setIF<Bits::Interrupts::VBLANK>();
    };

    auto updateLY = [&](uint8_t LY) {
        // write LY
        lcdIo.writeLY(LY);

        // update STAT's LYC=LY Flag
        uint8_t LYC = lcdIo.readLYC();
        uint8_t STAT = lcdIo.readSTAT();
        lcdIo.writeSTAT(set_bit<2>(STAT, LY == LYC));

        if ((get_bit<Bits::LCD::STAT::LYC_EQ_LY_INTERRUPT>(STAT)) && LY == LYC)
            interrupts.setIF<Bits::Interrupts::STAT>();
    };

    bool enabled = get_bit<LCD_ENABLE>(lcdIo.readLCDC());

    if (on && !enabled) {
        on = false;
        lcd.turnOff();
        updateLY(0);
        fetcherClear(); // TODO: bad...
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

    if (state == OAMScan) {
        uint8_t LY = lcdIo.readLY();

        uint8_t oamNum = dots / 2;
        if (dots % 2 == 0) {
            // read oam entry y
            scratchpad.oam.y = oam.read(4 * oamNum);
        }
        else {
            uint8_t x = oam.read(4 * oamNum + 1);
            int oamY = scratchpad.oam.y - 16;
            if (oamY <= LY && LY < oamY + 8)
                oamEntries.emplace_back(dots / 2, x, scratchpad.oam.y);
        }
        dots++;;
        if (dots == 80) {
            // end of OAM scan
            LX = 0;
            fetcher.state = FetcherState::Prefetcher; // TODO: bad not here
            fetcher.oamEntriesHit.clear();
            pixelSliceFetcher.dots = 0;
            bgPrefetcher.dots = 0;
            bgPrefetcher.x8 = 0;
            objPrefetcher.dots = 0;
            bgFifo.pixels.clear();
            objFifo.pixels.clear();
            updateState(PixelTransfer);
        }
    } else if (state == PixelTransfer) {
        // shift pixel from fifo to lcd if bg fifo is not empty
        // and it's not blocked by the pixel fetcher due to sprite fetch

        bool pushedToLcd = false;

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

                uint8_t LY = lcdIo.readLY();

                // TODO: handle overlap
                if (objPixel)
                    pixel = *objPixel;

                lcd.pushPixel(fifo_pixel_to_lcd_pixel(pixel));
                pushedToLcd = true;
            }
        }

        fetcherTick();

        if (pushedToLcd) {
            LX++;

            if (LX >= 160) {
                // end of pixel transfer
                updateState(HBlank);
                // assert(dots >= 80 + 172 && dots <= 80 + 289);
            } else {
                // compute sprite on current LX
                fetcher.oamEntriesHit.clear();

                assert(fetcher.oamEntriesHit.empty());

                for (const auto &entry: oamEntries) {
                    uint8_t oamX = entry.x - 8;
                    if (oamX == LX) {
                        fetcher.oamEntriesHit.push_back(entry);
                    }
                }
            }

        }

        dots++;

    } else if (state == HBlank) {
        dots++;

        if (dots == 456) {
            // end of hblank
            dots = 0;
            LX = 0;
            uint8_t LY = lcdIo.readLY();
            LY++;
            updateLY(LY);
            if (LY == 144) {
                updateState(State::VBlank);
            } else {
                updateState(State::OAMScan);
                oamEntries.clear(); // TODO: in updateState?
            }
        }
    } else if (state == VBlank) {
        dots++;
        if (dots % 456 == 0) {
			uint8_t LY = lcdIo.readLY();
            LY++;
            if (LY < 154) {
                updateLY(LY);
            } else {
                // end of vblank
                dots = 0;
                updateState(State::OAMScan);
                oamEntries.clear(); // TODO: in updateState?
                updateLY(0);
            }
        }
    }

    tCycles++;
}

void PPU::fetcherClear() {
    fetcher.state = FetcherState::Prefetcher;
    fetcher.oamEntriesHit.clear();
    pixelSliceFetcher.dots = 0;
    bgPrefetcher.dots = 0;
    bgPrefetcher.x8 = 0;
    objPrefetcher.dots = 0;
    bgFifo.pixels.clear();
    objFifo.pixels.clear();
}

void PPU::fetcherTick() {
    // TODO: bad here
    auto getPixelSlicerFetcherPixel = [&](uint8_t b) {
        uint8_t color =
                (get_bit(pixelSliceFetcher.scratchpad.tileDataLow, b) ? 0b01 : 0b00) |
                (get_bit(pixelSliceFetcher.scratchpad.tileDataHigh, b) ? 0b10 : 0b00);
        FIFO::Pixel p {
            .color = color
        };
        return p;
    };

    if (fetcher.state == FetcherState::Prefetcher) {
        if (fetcher.targetFifo == FIFOType::Bg) {
            bgPrefetcherTick();

            if (bgPrefetcher.dots == 2) {
                bgPrefetcher.dots = 0;
                pixelSliceFetcher.in.tileDataAddr = bgPrefetcher.out.tileDataAddr;
                fetcher.state = FetcherState::PixelSliceFetcher;
                pixelSliceFetcher.dots = 0;
            }
        } else if (fetcher.targetFifo == FIFOType::Obj) {
            objPrefetcherTick();

            if (objPrefetcher.dots == 2) {
                objPrefetcher.dots = 0;
                pixelSliceFetcher.in.tileDataAddr = objPrefetcher.out.tileDataAddr;
                fetcher.state = FetcherState::PixelSliceFetcher;
                pixelSliceFetcher.dots = 0;
            }
        }
    } else if (fetcher.state == FetcherState::PixelSliceFetcher) {
        pixelSliceFetcherTick();
        if (pixelSliceFetcher.dots == 4)
            fetcher.state = FetcherState::Pushing;

        // TODO: oam read should overlap last bg push?
    } else if (fetcher.state == FetcherState::Pushing) {
        if (fetcher.targetFifo == FIFOType::Bg) {
            if (!fetcher.oamEntriesHit.empty()) {
                // oam entry hit waiting to be served
                // discard bg push?
                objPrefetcher.in.entry = pop(fetcher.oamEntriesHit);
                fetcher.state = FetcherState::Prefetcher;
                fetcher.targetFifo = FIFOType::Obj;
            } else {
                if (bgFifo.pixels.empty()) {
                    // push pixels
                    for (int b = 7; b >= 0; b--)
                        bgFifo.pixels.push_back(getPixelSlicerFetcherPixel(b));

                    bgPrefetcher.x8 = (bgPrefetcher.x8 + 1) % 20;

                    fetcher.state = FetcherState::Prefetcher;
                }
            }
        } else if (fetcher.targetFifo == FIFOType::Obj) {
            objFifo.pixels.clear();

            // TODO: should not access flags so badly from
            bool backwardPush = true;
            if (get_bit<Bits::OAM::Attributes::X_FLIP>(objPrefetcher.scratchpad.oamFlags))
                backwardPush = false;

            // TODO: not clear/push but merge
            if (backwardPush) {
                for (int b = 7; b >= 0; b--)
                    objFifo.pixels.push_back(getPixelSlicerFetcherPixel(b));
            } else {
                for (int b = 0; b < 8; b++)
                    objFifo.pixels.push_back(getPixelSlicerFetcherPixel(b));
            }

            fetcher.state = FetcherState::Prefetcher;

            if (!fetcher.oamEntriesHit.empty()) {
                // oam entry hit waiting to be served
                // discard bg push?
                objPrefetcher.in.entry = pop(fetcher.oamEntriesHit);
                fetcher.targetFifo = FIFOType::Obj;
            } else {
                fetcher.targetFifo = FIFOType::Bg;
            }

        }
    }
}

void PPU::bgPrefetcherTick() {
    assert(bgPrefetcher.dots < 2);
    assert(bgPrefetcher.x8 < 20);

    if (bgPrefetcher.dots == 0) {
        uint8_t SCX = lcdIo.readSCX();
        bgPrefetcher.scratchpad.tilemapX = (bgPrefetcher.x8 + (SCX / 8)) % 32;
        bgPrefetcher.dots++;
    } else if (bgPrefetcher.dots == 1) {
        uint8_t SCY = lcdIo.readSCY();
        uint8_t LY = lcdIo.readLY(); // or fetcher.y?
        uint8_t tilemapY = ((LY + SCY) / 8) % 32;

        // fetch tile from tilemap
        uint16_t base = 0x9800;
        bgPrefetcher.scratchpad.tilemapAddr = base + 32 * tilemapY + bgPrefetcher.scratchpad.tilemapX;
        bgPrefetcher.scratchpad.tileNumber = vram.read(bgPrefetcher.scratchpad.tilemapAddr - MemoryMap::VRAM::START); // TODO: bad

        if (get_bit<Bits::LCD::LCDC::BG_WIN_TILE_DATA>(lcdIo.readLCDC()))
            bgPrefetcher.scratchpad.tileAddr = 0x8000 + bgPrefetcher.scratchpad.tileNumber * 16 /* sizeof tile */;
        else {
            if (bgPrefetcher.scratchpad.tileNumber < 128U)
                bgPrefetcher.scratchpad.tileAddr = 0x9000 + bgPrefetcher.scratchpad.tileNumber * 16 /* sizeof tile */;
            else
                bgPrefetcher.scratchpad.tileAddr = 0x8800 + (bgPrefetcher.scratchpad.tileNumber - 128U) * 16 /* sizeof tile */;
        }

        uint16_t tileY = (LY + SCY) % 8;
        bgPrefetcher.out.tileDataAddr = bgPrefetcher.scratchpad.tileAddr + tileY * 2 /* sizeof tile row */;;

        bgPrefetcher.dots++;
    }
}

void PPU::objPrefetcherTick() {
    assert(objPrefetcher.dots < 2);

    if (objPrefetcher.dots == 0) {
        objPrefetcher.scratchpad.tileNumber = oam.read(4 * objPrefetcher.in.entry.number + 2);
        objPrefetcher.dots++;
    } else if (objPrefetcher.dots == 1) {
        objPrefetcher.scratchpad.oamFlags  = oam.read(4 * objPrefetcher.in.entry.number + 3);
        objPrefetcher.scratchpad.tileAddr = 0x8000 + objPrefetcher.scratchpad.tileNumber * 16 /* sizeof tile */;

        int oamY = objPrefetcher.in.entry.y - 16;
        int yOffset = lcdIo.readLY() - oamY;

        objPrefetcher.out.tileDataAddr = objPrefetcher.scratchpad.tileAddr + yOffset * 2;
        objPrefetcher.dots++;
    }
}


void PPU::pixelSliceFetcherTick() {
    // GetTileDataLow
    if (pixelSliceFetcher.dots == 0) {
        pixelSliceFetcher.scratchpad.tileDataLow = vram.read(pixelSliceFetcher.in.tileDataAddr - MemoryMap::VRAM::START);
        pixelSliceFetcher.dots++;
    } else if (pixelSliceFetcher.dots == 1) {
        // TODO: nop?
        pixelSliceFetcher.dots++;
    }
    // GetTileDataHigh
    else if (pixelSliceFetcher.dots == 2) {
        pixelSliceFetcher.scratchpad.tileDataHigh = vram.read(pixelSliceFetcher.in.tileDataAddr - MemoryMap::VRAM::START + 1);
        pixelSliceFetcher.dots++;
    }
    else if (pixelSliceFetcher.dots == 3) {
        // TODO: nop?
        pixelSliceFetcher.dots++;
    }
}

bool PPU::isFifoBlocked() const {
    return !fetcher.oamEntriesHit.empty() || fetcher.targetFifo == FIFOType::Obj;
}

