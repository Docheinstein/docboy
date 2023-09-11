
#include "slot.h"

uint8_t DebuggableCartridgeSlot::read(uint16_t index) const {
    __try {
        uint8_t value = CartridgeSlot::read(index);
        if (robserver)
            robserver->onRead(index, value);
        return value;
    }
    __catch(const ReadMemoryException& ex) {
        if (robserver) {
            robserver->onReadError(index, ex.what());
            return 0;
        }
        __throw_exception_again;
    }
}

void DebuggableCartridgeSlot::write(uint16_t index, uint8_t value) {
    __try {
        uint8_t oldValue = CartridgeSlot::read(index);
        CartridgeSlot::write(index, value);
        if (wobserver)
            wobserver->onWrite(index, oldValue, value);
    }
    __catch(const WriteMemoryException& ex) {
        if (wobserver)
            wobserver->onWriteError(index, ex.what());
        else
            __throw_exception_again;
    }
}

void DebuggableCartridgeSlot::setObserver(IMemoryDebug::Observer* o) {
    robserver = o;
    wobserver = o;
}

void DebuggableCartridgeSlot::setObserver(IReadableDebug::Observer* o) {
    robserver = o;
}

void DebuggableCartridgeSlot::setObserver(IWritableDebug::Observer* o) {
    wobserver = o;
}
