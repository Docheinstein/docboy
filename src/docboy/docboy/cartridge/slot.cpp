#ifdef ENABLE_DEBUGGER
#include "docboy/cartridge/slot.h"
#include "docboy/debugger/memwatcher.h"

uint8_t CartridgeSlot::read_rom(uint16_t address) const {
    uint8_t value = cartridge->read_rom(address);
    DebuggerMemoryWatcher::notify_read(address, value);
    return value;
}

void CartridgeSlot::write_rom(uint16_t address, uint8_t value) {
    uint8_t old_value = cartridge->read_rom(address);
    cartridge->write_rom(address, value);
    DebuggerMemoryWatcher::notify_write(address, old_value, value);
}

uint8_t CartridgeSlot::read_ram(uint16_t address) const {
    uint8_t value = cartridge->read_ram(address);
    DebuggerMemoryWatcher::notify_read(address, value);
    return value;
}

void CartridgeSlot::write_ram(uint16_t address, uint8_t value) {
    uint8_t old_value = cartridge->read_ram(address);
    cartridge->write_ram(address, value);
    DebuggerMemoryWatcher::notify_write(address, old_value, value);
}
#endif