#ifndef MEMSNIFFER_H
#define MEMSNIFFER_H

#include <cstdint>

class DebuggerBackend;

class DebuggerMemorySniffer {
public:
    static void setObserver(DebuggerBackend* observer);
    static void notifyMemoryRead(uint16_t address, uint8_t value);
    static void notifyMemoryWrite(uint16_t address, uint8_t oldValue, uint8_t newValue);

private:
    static DebuggerBackend* observer;
};

#endif // MEMSNIFFER_H