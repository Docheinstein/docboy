#include "memsniffer.h"
#include "docboy/debugger/backend.h"
#include "utils/asserts.h"

DebuggerBackend* DebuggerMemorySniffer::observer {};

void DebuggerMemorySniffer::notifyMemoryRead(uint16_t address, uint8_t value) {
    if (observer)
        observer->onMemoryRead(address, value);
}

void DebuggerMemorySniffer::notifyMemoryWrite(uint16_t address, uint8_t oldValue, uint8_t newValue) {
    if (observer)
        observer->onMemoryWrite(address, oldValue, newValue);
}

void DebuggerMemorySniffer::setObserver(DebuggerBackend* o) {
    observer = o;
}
