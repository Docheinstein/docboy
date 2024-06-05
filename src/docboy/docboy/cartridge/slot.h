#ifndef SLOT_H
#define SLOT_H

#include <cstdint>
#include <memory>

#include "docboy/cartridge/cartridge.h"

#include "utils/asserts.h"

class CartridgeSlot {
public:
    void attach(std::unique_ptr<ICartridge> cart) {
        cartridge = std::move(cart);
    }

    void detach() {
        cartridge = nullptr;
    }

#ifdef ENABLE_DEBUGGER
    uint8_t read_rom(uint16_t address) const;
    void write_rom(uint16_t address, uint8_t value);

    uint8_t read_ram(uint16_t address) const;
    void write_ram(uint16_t address, uint8_t value);
#else
    uint8_t read_rom(uint16_t address) const {
        return cartridge->read_rom(address);
    }

    void write_rom(uint16_t address, uint8_t value) const {
        cartridge->write_rom(address, value);
    }

    uint8_t read_ram(uint16_t address) const {
        return cartridge->read_ram(address);
    }

    void write_ram(uint16_t address, uint8_t value) const {
        cartridge->write_ram(address, value);
    }
#endif

    void tick() const {
        cartridge->tick();
    }

    std::unique_ptr<ICartridge> cartridge {};
};
#endif // SLOT_H