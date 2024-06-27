#ifndef EXTBUS_H
#define EXTBUS_H

#include "docboy/bus/bus.h"
#include "docboy/memory/fwd/wram1fwd.h"
#include "docboy/memory/fwd/wram2fwd.h"

class CartridgeSlot;

class ExtBus final : public Bus {
public:
    ExtBus(CartridgeSlot& cartridge_slot, Wram1& wram1, Wram2& wram2);

private:
    CartridgeSlot& cartridge_slot;
    Wram1& wram1;
    Wram2& wram2;
};

#endif // EXTBUS_H