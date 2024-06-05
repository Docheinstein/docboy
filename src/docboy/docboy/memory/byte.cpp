#ifdef ENABLE_DEBUGGER

#include "docboy/memory/byte.h"
#include "docboy/debugger/memsniffer.h"

#include "utils/asserts.h"

byte& byte::operator=(uint8_t value) {
    ASSERT(address != INVALID_MEMORY_ADDRESS);
    uint8_t old_value = data;
    data = value;
    DebuggerMemorySniffer::notify_memory_write(address, old_value, value);
    return *this;
}

byte::operator uint8_t() const {
    ASSERT(address != INVALID_MEMORY_ADDRESS);
    DebuggerMemorySniffer::notify_memory_read(address, data);
    return data;
}

byte& byte::operator++() {
    uint8_t old_value = data;
    ++data;
    DebuggerMemorySniffer::notify_memory_write(address, old_value, data);
    return *this;
}

byte& byte::operator+=(uint64_t value) {
    uint8_t old_value = data;
    data += value;
    DebuggerMemorySniffer::notify_memory_write(address, old_value, data);
    return *this;
}

byte& byte::operator--() {
    uint8_t old_value = data;
    --data;
    DebuggerMemorySniffer::notify_memory_write(address, old_value, data);
    return *this;
}

byte& byte::operator-=(uint64_t value) {
    uint8_t old_value = data;
    data -= value;
    DebuggerMemorySniffer::notify_memory_write(address, old_value, data);
    return *this;
}

byte& byte::operator|=(uint64_t value) {
    uint8_t old_value = data;
    data |= value;
    DebuggerMemorySniffer::notify_memory_write(address, old_value, data);
    return *this;
}

byte& byte::operator&=(uint64_t value) {
    uint8_t old_value = data;
    data &= value;
    DebuggerMemorySniffer::notify_memory_write(address, old_value, data);
    return *this;
}

uint8_t byte::operator|(uint64_t value) const {
    uint8_t new_value = data | value;
    DebuggerMemorySniffer::notify_memory_read(address, data);
    return new_value;
}

uint8_t byte::operator&(uint64_t value) const {
    uint8_t new_value = data & value;
    DebuggerMemorySniffer::notify_memory_read(address, data);
    return new_value;
}

uint8_t byte::operator|(const byte& other) const {
    return static_cast<uint8_t>(*this) | static_cast<uint8_t>(other);
}

uint8_t byte::operator&(const byte& other) const {
    return static_cast<uint8_t>(*this) & static_cast<uint8_t>(other);
}

#endif