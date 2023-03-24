
#include "slot.h"

uint8_t DebuggableCartridgeSlot::read(uint16_t index) const {
    uint8_t value = CartridgeSlot::read(index);
    if (robserver)
        robserver->onRead(index, value);
    return value;
}

void DebuggableCartridgeSlot::write(uint16_t index, uint8_t value) {
    uint8_t oldValue = CartridgeSlot::read(index);
    CartridgeSlot::write(index, value);
    if (wobserver)
        wobserver->onWrite(index, oldValue, value);
}

void DebuggableCartridgeSlot::setObserver(IMemoryDebug::Observer *o) {
    robserver = o;
    wobserver = o;
}

void DebuggableCartridgeSlot::setObserver(IReadableDebug::Observer *o) {
    robserver = o;
}

void DebuggableCartridgeSlot::setObserver(IWritableDebug::Observer *o) {
    wobserver = o;
}
