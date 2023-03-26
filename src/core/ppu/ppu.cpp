#include "ppu.h"
#include "utils/binutils.h"
#include "core/definitions.h"
#include "core/memory/memory.h"
#include "core/lcd/lcd.h"
#include "core/io/interrupts.h"

using namespace Bits::LCD::LCDC;

PPU::PPU(ILCD &lcd, ILCDIO &lcdIo, IInterruptsIO &interrupts, IMemory &vram, IMemory &oam) :
    lcd(lcd), lcdIo(lcdIo), interrupts(interrupts),
    vram(vram), oam(oam),
    on(),
    state(OAMScan),
    bgFifo(), objFifo(),
    transferredPixels(),
    fetcher(),
    dots(), tCycles() {

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
        bgFifo.pixels.clear(); // TODO: ?
        dots = 0; // TODO: ?
        updateState(OAMScan); // TODO: here or on off -> on transition?
    } else if (!on && enabled) {
        on = true;
        lcd.turnOn();
    }

    if (!on)
        return; // TODO: ok?

    if (state == OAMScan) {
        dots++;
        if (dots == 80) {
            // end of OAM scan
            transferredPixels = 0;
            updateState(PixelTransfer);
        }
    } else if (state == PixelTransfer) {
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
            updateState(HBlank);
        }
    } else if (state == HBlank) {
        dots++;

        if (dots == 456) {
            // end of hblank
            dots = 0;
            uint8_t LY = lcdIo.readLY();
            LY++;
            updateLY(LY);
            if (LY == 144) {
                updateState(State::VBlank);
            } else {
                updateState(State::OAMScan);
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
                updateLY(0);
            }
        }
    }

    tCycles++;
}

void PPU::fetcherTick() {

    // GetTile
    if (fetcher.dots == 0) {
        uint8_t SCX = lcdIo.readSCX();
        fetcher.scratchpad.tilemapX = (fetcher.x8 + (SCX / 8)) % 32;
        fetcher.dots++;
    }
    else if (fetcher.dots == 1) {
        uint8_t SCY = lcdIo.readSCY();
        uint8_t tilemapY = ((fetcher.y + SCY) / 8) % 32;

        // fetch tile from tilemap
        uint16_t base = 0x9800;
        fetcher.scratchpad.tilemapAddr = base + 32 * tilemapY + fetcher.scratchpad.tilemapX;
        fetcher.scratchpad.tileNumber = vram.read(fetcher.scratchpad.tilemapAddr - MemoryMap::VRAM::START); // TODO: bad

        fetcher.dots++;
    }
    // GetTileDataLow
    else if (fetcher.dots == 2) {
        uint8_t SCX = lcdIo.readSCX();
        uint8_t SCY = lcdIo.readSCY();
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

void PPU::fetcherClear() {
    fetcher.dots = 0;
    fetcher.x8 = 0;
    fetcher.y = 0;
}


