#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
#include "byte.hpp"
#include "docboy/debugger/memsniffer.h"
#include "utils/asserts.h"
#include <iostream>

byte::byte(uint16_t memoryAddress, uint8_t initialValue) :
    data(initialValue) {
    setMemoryAddress(memoryAddress);
}

void byte::setMemoryAddress(uint16_t addr) {
    address = addr;
}

byte& byte::operator=(uint8_t value) {
    check(address != INVALID_MEMORY_ADDRESS);
    uint8_t oldValue = data;
    data = value;
    DebuggerMemorySniffer::notifyMemoryWrite(address, oldValue, value);
    return *this;
}

byte::operator uint8_t() const {
    check(address != INVALID_MEMORY_ADDRESS);
    DebuggerMemorySniffer::notifyMemoryRead(address, data);
    return data;
}

byte& byte::operator++() {
    uint8_t oldValue = data;
    ++data;
    DebuggerMemorySniffer::notifyMemoryWrite(address, oldValue, data);
    return *this;
}

byte& byte::operator|=(uint64_t value) {
    uint8_t oldValue = data;
    data |= value;
    DebuggerMemorySniffer::notifyMemoryWrite(address, oldValue, data);
    return *this;
}

byte& byte::operator&=(uint64_t value) {
    uint8_t oldValue = data;
    data &= value;
    DebuggerMemorySniffer::notifyMemoryWrite(address, oldValue, data);
    return *this;
}

uint8_t byte::operator|(uint64_t value) const {
    uint8_t newValue = data | value;
    DebuggerMemorySniffer::notifyMemoryRead(address, data);
    return newValue;
}

uint8_t byte::operator&(uint64_t value) const {
    uint8_t newValue = data & value;
    DebuggerMemorySniffer::notifyMemoryRead(address, data);
    return newValue;
}

uint8_t byte::operator|(const byte& other) const {
    return static_cast<uint8_t>(*this) | static_cast<uint8_t>(other);
}

uint8_t byte::operator&(const byte& other) const {
    return static_cast<uint8_t>(*this) & static_cast<uint8_t>(other);
}

#endif