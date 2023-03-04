#include "gameboybuilder.h"

GameBoyBuilder &GameBoyBuilder::setBootROM(std::unique_ptr<IBootROM> b) {
    bootRom = std::move(b);
    return *this;
}

GameBoyBuilder &GameBoyBuilder::setDisplay(std::unique_ptr<IDisplay> d) {
    display = std::move(d);
    return *this;
}

GameBoy GameBoyBuilder::build() {
    GameBoy gb;
    gb.display = std::move(display);
    gb.bus = std::make_unique<Bus>(gb.vram, gb.wram1, gb.wram2, gb.oam, gb.io, gb.hram, gb.ie);
    gb.cpu = std::make_unique<GameBoy::CPU>(*gb.bus, std::move(bootRom));
    gb.gpu = std::make_unique<GPU>(*gb.display, gb.vram, gb.io);
    return gb;
}

