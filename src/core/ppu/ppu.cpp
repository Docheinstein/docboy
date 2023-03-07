#include "ppu.h"
#include "utils/binutils.h"
#include "core/definitions.h"
#include "core/io.h"
#include "lcd.h"

using namespace Bits::Registers::LCD::LCDC;

PPU::PPU(ILCD &lcd, IMemory &vram, IMemory &oam, IIO &io) :
    lcd(lcd), vram(vram), oam(oam), io(io),
    bgFifo(), objFifo(),
    state(State::OAMScan),
    transferredPixels(),
    fetcher(),
    dots(), tCycles() {

}

void PPU::tick() {
    uint8_t LCDC = io.readLCDC();
    bool enabled = get_bit<LCD_ENABLE>(LCDC);

    if (lcd.isOn() && !enabled)
        lcd.turnOff();
    else if (!lcd.isOn() && enabled)
        lcd.turnOn();

    if (!enabled)
        return; // TODO: ok?

    if (state == State::OAMScan) {
        dots++;
        if (dots == 80) {
            // end of OAM scan
            transferredPixels = 0;
            state = State::PixelTransfer;
        }
    } else if (state == State::PixelTransfer) {
        // shift pixel from fifo to lcd if there are at least 8 pixels
        if (transferredPixels < 160 && bgFifo.pixels.size() > 8) {
            auto p = bgFifo.pixels.front();
            bgFifo.pixels.pop_front();
            // TODO: so bad
            // TODO: palette
            ILCD::Pixel lcdPixel;
            if (p.color == 0)
                lcdPixel = ILCD::Pixel::Color0;
            else if (p.color == 1)
                lcdPixel = ILCD::Pixel::Color1;
            else if (p.color == 2)
                lcdPixel = ILCD::Pixel::Color2;
            else if (p.color == 3)
                lcdPixel = ILCD::Pixel::Color3;
            else
                throw std::runtime_error("unexpected color pixel: " + std::to_string(p.color));
            lcd.pushPixel(lcdPixel);
            transferredPixels++;
        }

        fetcherTick();


        dots++;
        if (dots == 80 + 289) {
            // end of pixel transfer
            state = State::HBlank;
        }
    } else if (state == State::HBlank) {
        dots++;

        if (dots == 456) {
            // end of hblank
            dots = 0;
            uint8_t LY = io.readLY();
            LY++;
            io.writeLY(LY);
            if (LY == 144) {
                // begin vblank
                state = State::VBlank;
            } else {
                state = State::OAMScan;
            }
        }
    } else if (state == State::VBlank) {
        dots++;
        if (dots % 456 == 0) {
            uint8_t LY = io.readLY();
            LY++;
            if (LY < 154) {
                io.writeLY(LY);
            } else {
                // end of vblank
                dots = 0;
                state = State::OAMScan;
                io.writeLY(0);
            }
        }
    }

    tCycles++;
}

void PPU::fetcherTick() {

    // GetTile
    if (fetcher.dots == 0) {
        uint8_t SCX = io.readSCX();
        fetcher.scratchpad.tilemapX = (fetcher.x8 + (SCX / 8)) % 32;
        fetcher.dots++;
    }
    else if (fetcher.dots == 1) {
        uint8_t SCY = io.readSCY();
        uint8_t tilemapY = ((fetcher.y + SCY) / 8) % 32;

        // fetch tile from tilemap
        uint16_t base = 0x9800;
        fetcher.scratchpad.tilemapAddr = base + 32 * tilemapY + fetcher.scratchpad.tilemapX;
        fetcher.scratchpad.tileNumber = vram.read(fetcher.scratchpad.tilemapAddr - MemoryMap::VRAM::START); // TODO: bad

        fetcher.dots++;
    }
    // GetTileDataLow
    else if (fetcher.dots == 2) {
        uint8_t SCX = io.readSCX();
        uint8_t SCY = io.readSCY();
        uint16_t tileY = (fetcher.y + SCY) % 8;
        fetcher.scratchpad.tileAddr = 0x8000 + fetcher.scratchpad.tileNumber * 16 /* sizeof tile */;
        fetcher.scratchpad.tileDataAddr = fetcher.scratchpad.tileAddr + tileY * 2;
        fetcher.scratchpad.tileDataLow = vram.read(fetcher.scratchpad.tileDataAddr - MemoryMap::VRAM::START);
        fetcher.dots++;
    } else if (fetcher.dots == 3) {
        // TODO: nop?
        fetcher.dots++;
    }
    // GetTileDataHigh
    else if (fetcher.dots == 4) {
        fetcher.scratchpad.tileDataHigh = vram.read(fetcher.scratchpad.tileDataAddr - MemoryMap::VRAM::START + 1);
        fetcher.dots++;
    }
    else if (fetcher.dots == 5) {
        // TODO: nop?
        fetcher.dots++;
    }
    // Push
    else {
        if (bgFifo.pixels.size() <= 8) {
            for (int b = 7; b >= 0; b--) {
                uint8_t color =
                        (get_bit(fetcher.scratchpad.tileDataLow, b) ? 0b10 : 0b00) |
                        (get_bit(fetcher.scratchpad.tileDataHigh, b) ? 0b01 : 0b00);
                FIFO::Pixel p {
                    .color = color
                };
                bgFifo.pixels.push_back(p);
            }
            fetcher.dots = 0;
            fetcher.x8 = (fetcher.x8 + 1) % 20;
            if (fetcher.x8 == 0)
                fetcher.y = (fetcher.y + 1) % 144;
        } else {
            fetcher.dots++;
        }
    }
}