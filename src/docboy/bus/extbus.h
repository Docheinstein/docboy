#ifndef EXTBUS_H
#define EXTBUS_H

#include "bus.h"
#include "docboy/bootrom/macros.h"
#include "docboy/memory/fwd/wram1fwd.h"
#include "docboy/memory/fwd/wram2fwd.h"

class CartridgeSlot;

class ExtBus : public Bus<ExtBus> {
    friend class Bus<ExtBus>;

public:
    ExtBus(CartridgeSlot& cartridgeSlot, Wram1& wram1, Wram2& wram2);

private:
    [[nodiscard]] uint8_t readCartridgeRom(uint16_t address) const;
    void writeCartridgeRom(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readCartridgeRam(uint16_t address) const;
    void writeCartridgeRam(uint16_t address, uint8_t value);

    CartridgeSlot& cartridgeSlot;
    Wram1& wram1;
    Wram2& wram2;
};

#endif // EXTBUS_H