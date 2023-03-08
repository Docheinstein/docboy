#include "gameboy.h"
#include "clock/clockable.h"

GameBoy::GameBoy(std::unique_ptr<Impl::ILCD> lcd,
                 std::unique_ptr<Impl::IBootROM> bootRom,
                 uint64_t cpuFreq,
                 uint64_t ppuFreq) :
    vram(MemoryMap::VRAM::SIZE),
    wram1(MemoryMap::WRAM1::SIZE),
    wram2(MemoryMap::WRAM2::SIZE),
    oam(MemoryMap::OAM::SIZE),
    io(),
    hram(MemoryMap::HRAM::SIZE),
    ie(1),
    bus(vram, wram1, wram2, oam, io, hram, io),
    serialPort(bus),
    cpu(bus, serialPort, std::move(bootRom)),
    lcd(std::move(lcd)),
    ppu(*lcd, vram, oam, io),
    clock(std::initializer_list<std::pair<IClockable *, uint64_t>>{
        std::make_pair(&cpu, cpuFreq),
        std::make_pair(&ppu, ppuFreq)
    }) {

}

GameBoy::Builder &GameBoy::Builder::setFrequency(uint64_t frequency_) {
    frequency = frequency_;
    return *this;
}

GameBoy::Builder &GameBoy::Builder::setBootROM(std::unique_ptr<Impl::IBootROM> bootRom_) {
    bootRom = std::move(bootRom_);
    return *this;
}

GameBoy::Builder &GameBoy::Builder::setLCD(std::unique_ptr<Impl::ILCD> lcd_) {
    lcd = std::move(lcd_);
    return *this;
}

GameBoy GameBoy::Builder::build() {
    uint64_t ppuFreq = frequency != 0 ? frequency : Specs::PPU::FREQUENCY;
    uint64_t cpuFreq = ppuFreq / (Specs::PPU::FREQUENCY / Specs::CPU::FREQUENCY);
    return GameBoy(std::move(lcd), std::move(bootRom), cpuFreq, ppuFreq);
}

void GameBoy::attachCartridge(std::unique_ptr<Cartridge> c) {
    cartridge = std::move(c);
    bus.attachCartridge(cartridge.get());
}

void GameBoy::detachCartridge() {
    cartridge = nullptr;
    bus.detachCartridge();
}

void GameBoy::attachSerialLink(SerialLink::Plug &plug) {
    serialPort.attachSerialLink(plug);
}

void GameBoy::detachSerialLink() {
    serialPort.detachSerialLink();
}