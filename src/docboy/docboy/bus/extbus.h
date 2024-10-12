#ifndef EXTBUS_H
#define EXTBUS_H

#include "docboy/bus/bus.h"
#include "docboy/memory/fwd/wram1fwd.h"
#include "docboy/memory/fwd/wram2fwd.h"

class CartridgeSlot;

class ExtBus final : public Bus {
public:
    ExtBus(CartridgeSlot& cartridge_slot, Wram1& wram1, Wram2* wram2);

#ifdef ENABLE_CGB
    void set_wram2_bank(uint8_t bank);
#endif

private:
#if ENABLE_CGB
    uint8_t read_wram2(uint16_t address) const;
    void write_wram2(uint16_t address, uint8_t value);
#endif

    CartridgeSlot& cartridge_slot;
    Wram1& wram1;
    Wram2* wram2;

#ifdef ENABLE_CGB
    uint8_t wram2_bank {};
#endif
};

#endif // EXTBUS_H