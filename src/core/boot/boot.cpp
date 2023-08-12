#include "boot.h"

Boot::Boot(std::unique_ptr<IBootROM> bootRom_) :
    BOOT(bootRom_ != nullptr ? 0 : 1) {
    bootRom = std::move(bootRom_);
}

void Boot::loadState(IReadableState& state) {
    // no need to save/load BOOT flag
}

void Boot::saveState(IWritableState& state) {
    // no need to save/load BOOT flag
}

uint8_t Boot::readBOOT() const {
    return BOOT;
}

void Boot::writeBOOT(uint8_t value) {
    BOOT = value;
}

uint8_t Boot::read(uint16_t index) const {
    if (bootRom)
        return bootRom->read(index);
    throw std::runtime_error("Cannot read from boot ROM: not initialized");
}