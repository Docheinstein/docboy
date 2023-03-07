#include "gameboybuilder.h"

GameBoyBuilder &GameBoyBuilder::setBootROM(std::unique_ptr<IBootROM> b) {
    bootRom = std::move(b);
    return *this;
}

GameBoyBuilder &GameBoyBuilder::setLCD(std::unique_ptr<ILCDImpl> lcd_) {
    lcd = std::move(lcd_);
    return *this;
}

GameBoy GameBoyBuilder::build() {
    GameBoy gb;
    gb.lcd = std::move(lcd);
    gb.bus = std::make_unique<Bus>(gb.vram, gb.wram1, gb.wram2, gb.oam, gb.io, gb.hram, gb.ie);
    gb.cpu = std::make_unique<CPUImpl>(*gb.bus, std::move(bootRom));
    gb.ppu = std::make_unique<PPUImpl>(*gb.lcd, gb.vram, gb.oam, gb.io);
    return gb;
}

