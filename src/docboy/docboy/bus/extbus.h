#ifndef EXTBUS_H
#define EXTBUS_H

#include "docboy/bus/bus.h"
#include "docboy/memory/fwd/wram1fwd.h"
#include "docboy/memory/fwd/wram2fwd.h"

class CartridgeSlot;

class ExtBus final : public Bus<ExtBus> {
public:
    ExtBus(CartridgeSlot& cartridge_slot, Wram1& wram1, Wram2& wram2);

private:
    uint8_t read_cartridge_rom(uint16_t address) const;
    void write_cartridge_rom(uint16_t address, uint8_t value);

    uint8_t read_cartridge_ram(uint16_t address) const;
    void write_cartridge_ram(uint16_t address, uint8_t value);

    CartridgeSlot& cartridge_slot;
    Wram1& wram1;
    Wram2& wram2;
};

#endif // EXTBUS_H