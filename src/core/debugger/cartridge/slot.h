#ifndef DEBUGGERCARTRIDGESLOT_H
#define DEBUGGERCARTRIDGESLOT_H

#include "core/cartridge/slot.h"
#include "core/debugger//memory/memory.h"

class DebuggableCartridgeSlot : public CartridgeSlot, public IMemoryDebug {
public:
    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

    void setObserver(IReadableDebug::Observer* o) override;
    void setObserver(IWritableDebug::Observer* o) override;
    void setObserver(IMemoryDebug::Observer* o) override;

private:
    IReadableDebug::Observer* robserver;
    IWritableDebug::Observer* wobserver;
};

#endif // DEBUGGERCARTRIDGESLOT_H