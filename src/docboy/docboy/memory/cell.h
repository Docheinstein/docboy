#ifndef CELL_H
#define CELL_H

#include "docboy/memory/fwd/cellfwd.h"

#ifdef ENABLE_DEBUGGER

#include <functional>

#include "docboy/debugger/memwatcher.h"
#include "docboy/memory/traits.h"

#include "utils/asserts.h"

struct UInt8 {
    static constexpr uint32_t INVALID_MEMORY_ADDRESS = UINT32_MAX;
    static constexpr uint32_t NOT_WATCHED_MEMORY_ADDRESS = INVALID_MEMORY_ADDRESS - 1;

    UInt8() = default;

    constexpr explicit UInt8(uint32_t address) :
        address {address} {
    }

    UInt8(const UInt8& b) = delete;
    UInt8(UInt8&& b) = delete;
    UInt8& operator=(const UInt8& b) = delete;
    UInt8& operator=(UInt8&& b) = delete;

    operator uint8_t() const {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_read();
        return data;
    }

    UInt8& operator=(uint8_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_write();
        data = value;
        return *this;
    }

    UInt8& operator++() {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_write();
        uint8_t new_value = data + 1;
        data = new_value;
        return *this;
    }

    UInt8& operator+=(uint64_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_write();
        uint8_t new_value = data += value;
        data = new_value;
        return *this;
    }

    UInt8& operator--() {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_write();
        uint8_t new_value = data - 1;
        data = new_value;
        return *this;
    }

    UInt8& operator-=(uint64_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_write();
        uint8_t new_value = data -= value;
        data = new_value;
        return *this;
    }

    UInt8& operator|=(uint64_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_write();
        uint8_t new_value = data | value;
        data = new_value;
        return *this;
    }

    UInt8& operator&=(uint64_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_write();
        uint8_t new_value = data & value;
        data = new_value;
        return *this;
    }

    uint8_t operator|(uint64_t value) const {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_read();
        uint8_t new_value = data | value;
        return new_value;
    }

    uint8_t operator&(uint64_t value) const {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        notify_read();
        uint8_t new_value = data & value;
        return new_value;
    }

    uint8_t operator|(const UInt8& other) const {
        return static_cast<uint8_t>(*this) | static_cast<uint8_t>(other);
    }

    uint8_t operator&(const UInt8& other) const {
        return static_cast<uint8_t>(*this) & static_cast<uint8_t>(other);
    }

    inline void unwatch() {
        address = NOT_WATCHED_MEMORY_ADDRESS;
    }

    uint32_t address {INVALID_MEMORY_ADDRESS};
    uint8_t data {};

private:
    inline void notify_read() const {
        if (address <= UINT16_MAX) {
            DebuggerMemoryWatcher::notify_read(address);
        }
    }

    inline void notify_write() const {
        if (address <= UINT16_MAX) {
            DebuggerMemoryWatcher::notify_write(address);
        }
    }
};

constexpr inline UInt8 make_uint8(const uint32_t address) {
    return UInt8 {address};
}

#else

constexpr inline UInt8 make_uint8(const uint16_t address) {
    return {};
}

#endif

template <uint32_t Address>
struct Composite {
public:
    using Bool = UInt8;

protected:
    static constexpr UInt8 make_uint8() {
        return ::make_uint8(Address);
    }

    static constexpr Bool make_bool() {
        return make_uint8();
    }
};

#endif // CELL_H