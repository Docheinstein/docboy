#ifndef EXTBUS_H
#define EXTBUS_H

#include "docboy/bus/bus.h"

class Wram1;
class Wram2;
class CartridgeSlot;

class ExtBus final : public Bus {
public:
    template <Device::Type Dev>
    using View = BusView<ExtBus, Dev>;

#ifdef ENABLE_CGB
    explicit ExtBus(CartridgeSlot& cartridge_slot);
#else
    ExtBus(CartridgeSlot& cartridge_slot, Wram1& wram1, Wram2* wram2);
#endif

    void reset();

private:
    CartridgeSlot& cartridge_slot;

#ifndef ENABLE_CGB
    Wram1& wram1;
    Wram2* wram2;
#endif
};

#endif // EXTBUS_H