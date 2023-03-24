#include "slot.h"
#include "cartridge.h"

void CartridgeSlot::attachCartridge(std::unique_ptr<ICartridge> cartridge_) {
    cartridge = std::move(cartridge_);
}

void CartridgeSlot::detachCartridge() {
    cartridge = nullptr;
}

uint8_t CartridgeSlot::read(uint16_t index) const {
    if (cartridge)
        return cartridge->read(index);
    return 0xFF;
}

void CartridgeSlot::write(uint16_t index, uint8_t value) {
    if (cartridge)
        cartridge->write(index, value);
}

ICartridge::Header CartridgeSlot::header() const {
    if (cartridge)
        return cartridge->header();
    return {};
}
