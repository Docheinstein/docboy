#include "debuggablecore.h"

DebuggableCore::DebuggableCore(GameBoy &gameboy) :
        Core(gameboy),
        observer(),
        bootromObserver(this, MemoryMap::ROM0::START),
        cartridgeObserver(this, MemoryMap::ROM0::START),
        vramObserver(this, MemoryMap::VRAM::START),
        wram1Observer(this, MemoryMap::WRAM1::START),
        wram2Observer(this, MemoryMap::WRAM2::START),
        oamObserver(this, MemoryMap::OAM::START),
        ioObserver(this, MemoryMap::IO::START),
        hramObserver(this, MemoryMap::HRAM::START),
        ieObserver(this, MemoryMap::IE) {
    gameboy.vram.setObserver(&vramObserver);
    gameboy.vram.setObserver(&vramObserver);
    gameboy.wram1.setObserver(&wram1Observer);
    gameboy.wram2.setObserver(&wram2Observer);
    gameboy.oam.setObserver(&oamObserver);
    gameboy.io.setObserver(&ioObserver);
    gameboy.hram.setObserver(&hramObserver);
    gameboy.ie.setObserver(&ieObserver);
    IBootROM *bootrom = gameboy.cpu->getBootROM();
    if (bootrom)
        bootrom->setObserver(&bootromObserver);
}

void DebuggableCore::setObserver(IDebuggableCore::Observer *obs) {
    observer = obs;
}

IDebuggableCPU &DebuggableCore::getCpu() {
    return *gameboy.cpu;
}

IDebuggablePPU &DebuggableCore::getPpu() {
    return *gameboy.ppu;
}

IBus &DebuggableCore::getBus() {
    return *gameboy.bus;
}

IDebuggableLCD &DebuggableCore::getLcd() {
    return *gameboy.lcd;
}

bool DebuggableCore::tick() {
    if (observer) {
        if (!observer->onTick(clk))
            stop();
    }
    return Core::tick();
}

void DebuggableCore::onRead(uint16_t addr, uint8_t value) {
    if (observer)
        observer->onMemoryRead(addr, value);
}

void DebuggableCore::onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) {
    if (observer)
        observer->onMemoryWrite(addr, oldValue, newValue);
}

void DebuggableCore::attachCartridge(std::unique_ptr<Cartridge> cartridge) {
    if (cartridge)
        cartridge->setObserver(&cartridgeObserver);
    Core::attachCartridge(std::move(cartridge));
}

DebuggableCore::MemoryObserver::MemoryObserver(DebuggableMemory::Observer *observer, uint16_t base)
    : observer(observer), base(base) {

}

void DebuggableCore::MemoryObserver::onRead(uint16_t addr, uint8_t value) {
    observer->onRead(base + addr, value);
}

void DebuggableCore::MemoryObserver::onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) {
    observer->onWrite(base + addr, oldValue, newValue);
}
