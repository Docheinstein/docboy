#ifdef ENABLE_DEBUGGER
#include "docboy/cartridge/slot.h"
#include "docboy/debugger/memwatcher.h"

uint8_t CartridgeSlot::read_rom(uint16_t address) const {
    DebuggerMemoryWatcher::notify_read(address);
    uint8_t value = cartridge->read_rom(address);
    return value;
}

void CartridgeSlot::write_rom(uint16_t address, uint8_t value) {
    DebuggerMemoryWatcher::notify_write(address);
    cartridge->write_rom(address, value);
}

uint8_t CartridgeSlot::read_ram(uint16_t address) const {
    DebuggerMemoryWatcher::notify_read(address);
    uint8_t value = cartridge->read_ram(address);
    return value;
}

void CartridgeSlot::write_ram(uint16_t address, uint8_t value) {
    DebuggerMemoryWatcher::notify_write(address);
    cartridge->write_ram(address, value);
}
#endif