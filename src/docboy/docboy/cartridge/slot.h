#ifndef SLOT_H
#define SLOT_H

#include "cartridge.h"
#include "docboy/shared//macros.h"
#include "utils/asserts.h"
#include <cstdint>
#include <memory>

class CartridgeSlot {
public:
    void attach(std::unique_ptr<ICartridge>&& cart) {
        cartridge = std::move(cart);
    }

    void detach() {
        cartridge = nullptr;
    }

#ifdef ENABLE_DEBUGGER
    [[nodiscard]] uint8_t readRom(uint16_t address) const;
    void writeRom(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readRam(uint16_t address) const;
    void writeRam(uint16_t address, uint8_t value);
#else
    [[nodiscard]] uint8_t readRom(uint16_t address) const {
        return cartridge->readRom(address);
    }

    void writeRom(uint16_t address, uint8_t value) const {
        cartridge->writeRom(address, value);
    }

    [[nodiscard]] uint8_t readRam(uint16_t address) const {
        return cartridge->readRam(address);
    }

    void writeRam(uint16_t address, uint8_t value) const {
        cartridge->writeRam(address, value);
    }
#endif

    std::unique_ptr<ICartridge> cartridge {};
};
#endif // SLOT_H