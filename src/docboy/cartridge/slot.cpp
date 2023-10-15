#ifdef ENABLE_DEBUGGER
#include "slot.h"
#include "docboy/debugger/memsniffer.h"

uint8_t CartridgeSlot::readRom(uint16_t address) const {
    uint8_t value = cartridge->readRom(address);
    DebuggerMemorySniffer::notifyMemoryRead(address, value);
    return value;
}

void CartridgeSlot::writeRom(uint16_t address, uint8_t value) {
    uint8_t oldValue = cartridge->readRom(address);
    cartridge->writeRom(address, value);
    DebuggerMemorySniffer::notifyMemoryWrite(address, oldValue, value);
}

uint8_t CartridgeSlot::readRam(uint16_t address) const {
    uint8_t value = cartridge->readRam(address);
    DebuggerMemorySniffer::notifyMemoryRead(address, value);
    return value;
}

void CartridgeSlot::writeRam(uint16_t address, uint8_t value) {
    uint8_t oldValue = cartridge->readRam(address);
    cartridge->writeRam(address, value);
    DebuggerMemorySniffer::notifyMemoryWrite(address, oldValue, value);
}

#endif