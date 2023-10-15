#include "core.h"
#include "docboy/cartridge/factory.h"
#include <iostream>

static constexpr uint32_t STATE_SAVE_SIZE_UNKNOWN = UINT32_MAX;
static uint32_t STATE_SAVE_SIZE = STATE_SAVE_SIZE_UNKNOWN;

Core::Core(GameBoy& gb) :
    gb(gb) {
}

void Core::loadRom(const std::string& filename) const {
    loadRom(CartridgeFactory().create(filename));
}

void Core::loadRom(std::unique_ptr<ICartridge>&& cartridge) const {
    gb.cartridgeSlot.attach(std::move(cartridge));
}

void Core::attachSerialLink(SerialLink::Plug& plug) const {
    gb.serialPort.attach(plug);
}

bool Core::canSaveRam() const {
    return getRamSaveSize() > 0;
}

uint32_t Core::getRamSaveSize() const {
    return gb.cartridgeSlot.cartridge->getRamSaveSize();
}

void Core::saveRam(void* data) const {
    memcpy(data, gb.cartridgeSlot.cartridge->getRamSaveData(), getRamSaveSize());
}

void Core::loadRam(const void* data) const {
    memcpy(gb.cartridgeSlot.cartridge->getRamSaveData(), data, getRamSaveSize());
}

uint32_t Core::getStateSaveSize() const {
    if (STATE_SAVE_SIZE == STATE_SAVE_SIZE_UNKNOWN)
        parcelizeState();
    check(STATE_SAVE_SIZE != STATE_SAVE_SIZE_UNKNOWN);
    return STATE_SAVE_SIZE;
}

void Core::saveState(void* data) const {
    memcpy(data, parcelizeState().getData(), getStateSaveSize());
}

void Core::loadState(const void* data) const {
    unparcelizeState(Parcel {data, getStateSaveSize()});
}

Parcel Core::parcelizeState() const {
    Parcel p;

    gb.cpu.saveState(p);
    gb.ppu.saveState(p);
    gb.cartridgeSlot.cartridge->saveState(p);
    gb.vram.saveState(p);
    gb.wram1.saveState(p);
    gb.wram2.saveState(p);
    gb.oam.saveState(p);
    gb.hram.saveState(p);
    gb.boot.saveState(p);
    gb.serialPort.saveState(p);
    gb.timers.saveState(p);
    gb.interrupts.saveState(p);
    gb.sound.saveState(p);
    gb.video.saveState(p);
    gb.lcd.saveState(p);
    gb.dma.saveState(p);

    STATE_SAVE_SIZE = p.getSize();

    return p;
}

void Core::unparcelizeState(Parcel&& p) const {
    gb.cpu.loadState(p);
    gb.ppu.loadState(p);
    gb.cartridgeSlot.cartridge->loadState(p);
    gb.vram.loadState(p);
    gb.wram1.loadState(p);
    gb.wram2.loadState(p);
    gb.oam.loadState(p);
    gb.hram.loadState(p);
    gb.boot.loadState(p);
    gb.serialPort.loadState(p);
    gb.timers.loadState(p);
    gb.interrupts.loadState(p);
    gb.sound.loadState(p);
    gb.video.loadState(p);
    gb.lcd.loadState(p);
    gb.dma.loadState(p);
    check(p.getRemainingSize() == 0);
}

#ifdef ENABLE_DEBUGGER
void Core::attachDebugger(DebuggerBackend& dbg) {
    debugger = &dbg;
}

void Core::detachDebugger() {
    debugger = nullptr;
}

bool Core::isDebuggerAskingToShutdown() const {
    return debugger != nullptr && debugger->isAskingToShutdown();
}
#endif