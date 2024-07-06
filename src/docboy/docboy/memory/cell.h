#ifndef CELL_H
#define CELL_H

#include "docboy/memory/fwd/cellfwd.h"

#ifdef ENABLE_DEBUGGER

#include <functional>

#include "docboy/debugger/memwatcher.h"
#include "docboy/memory/traits.h"

#include "utils/asserts.h"

constexpr inline uint32_t INVALID_MEMORY_ADDRESS = UINT32_MAX;

struct UInt8 {
    UInt8() = default;

    constexpr explicit UInt8(uint16_t address) :
        address {address} {
    }

    UInt8(const UInt8& b) = delete;
    UInt8(UInt8&& b) = delete;
    UInt8& operator=(const UInt8& b) = delete;
    UInt8& operator=(UInt8&& b) = delete;

    UInt8& operator=(uint8_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t old_value = data;
        data = value;
        DebuggerMemoryWatcher::notify_write(address, old_value, data);
        return *this;
    }

    operator uint8_t() const {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        DebuggerMemoryWatcher::notify_read(address, data);
        return data;
    }

    UInt8& operator++() {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t old_value = data;
        ++data;
        DebuggerMemoryWatcher::notify_write(address, old_value, data);
        return *this;
    }

    UInt8& operator+=(uint64_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t old_value = data;
        data += value;
        DebuggerMemoryWatcher::notify_write(address, old_value, data);
        return *this;
    }

    UInt8& operator--() {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t old_value = data;
        --data;
        DebuggerMemoryWatcher::notify_write(address, old_value, data);
        return *this;
    }

    UInt8& operator-=(uint64_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t old_value = data;
        data -= value;
        DebuggerMemoryWatcher::notify_write(address, old_value, data);
        return *this;
    }

    UInt8& operator|=(uint64_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t old_value = data;
        data |= value;
        DebuggerMemoryWatcher::notify_write(address, old_value, data);
        return *this;
    }

    UInt8& operator&=(uint64_t value) {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t old_value = data;
        data &= value;
        DebuggerMemoryWatcher::notify_write(address, old_value, data);
        return *this;
    }

    uint8_t operator|(uint64_t value) const {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t new_value = data | value;
        DebuggerMemoryWatcher::notify_read(address, data);
        return new_value;
    }

    uint8_t operator&(uint64_t value) const {
        ASSERT(address != INVALID_MEMORY_ADDRESS);
        uint8_t new_value = data & value;
        DebuggerMemoryWatcher::notify_read(address, data);
        return new_value;
    }

    uint8_t operator|(const UInt8& other) const {
        return static_cast<uint8_t>(*this) | static_cast<uint8_t>(other);
    }

    uint8_t operator&(const UInt8& other) const {
        return static_cast<uint8_t>(*this) & static_cast<uint8_t>(other);
    }

    uint32_t address {INVALID_MEMORY_ADDRESS};
    uint8_t data {};
};

constexpr inline UInt8 make_uint8(const uint16_t address) {
    return UInt8 {address};
}

template <typename T, uint16_t Address>
struct Composite {
public:
    struct UInt8 {
        UInt8() = default;

        explicit UInt8(Composite& composite) :
            composite {composite} {
        }

        UInt8(const UInt8& b) = delete;
        UInt8(UInt8&& b) = delete;
        UInt8& operator=(const UInt8& b) = delete;
        UInt8& operator=(UInt8&& b) = delete;

        UInt8& operator=(uint8_t value) {
            if (is_notification_enabled()) {
                uint8_t old_value = get_value();
                data = value;
                DebuggerMemoryWatcher::notify_write(Address, old_value, get_value());
            } else {
                data = value;
            }
            return *this;
        }

        operator uint8_t() const {
            if (is_notification_enabled()) {
                DebuggerMemoryWatcher::notify_read(Address, get_value());
            }
            return data;
        }

        UInt8& operator++() {
            if (is_notification_enabled()) {
                uint8_t old_value = get_value();
                ++data;
                DebuggerMemoryWatcher::notify_write(Address, old_value, get_value());
            } else {
                ++data;
            }
            return *this;
        }

        UInt8& operator+=(uint64_t value) {
            if (is_notification_enabled()) {
                uint8_t old_value = get_value();
                data += value;
                DebuggerMemoryWatcher::notify_write(Address, old_value, get_value());
            } else {
                data += value;
            }
            return *this;
        }

        UInt8& operator--() {
            if (is_notification_enabled()) {
                uint8_t old_value = get_value();
                --data;
                DebuggerMemoryWatcher::notify_write(Address, old_value, get_value());
            } else {
                --data;
            }
            return *this;
        }

        UInt8& operator-=(uint64_t value) {
            if (is_notification_enabled()) {
                uint8_t old_value = get_value();
                data -= value;
                DebuggerMemoryWatcher::notify_write(Address, old_value, get_value());
            } else {
                data -= value;
            }
            return *this;
        }

        UInt8& operator|=(uint64_t value) {
            if (is_notification_enabled()) {
                uint8_t old_value = get_value();
                data |= value;
                DebuggerMemoryWatcher::notify_write(Address, old_value, get_value());
            } else {
                data |= value;
            }
            return *this;
        }

        UInt8& operator&=(uint64_t value) {
            if (is_notification_enabled()) {
                uint8_t old_value = get_value();
                data &= value;
                DebuggerMemoryWatcher::notify_write(Address, old_value, get_value());
            } else {
                data &= value;
            }
            return *this;
        }

        uint8_t operator|(uint64_t value) const {
            uint8_t new_value = data | value;
            if (is_notification_enabled()) {
                DebuggerMemoryWatcher::notify_read(Address, get_value());
            }
            return new_value;
        }

        uint8_t operator&(uint64_t value) const {
            uint8_t new_value = data & value;
            if (is_notification_enabled()) {
                DebuggerMemoryWatcher::notify_read(Address, get_value());
            }
            return new_value;
        }

        uint8_t operator|(const UInt8& other) const {
            return static_cast<uint8_t>(*this) | static_cast<uint8_t>(other);
        }

        uint8_t operator&(const UInt8& other) const {
            return static_cast<uint8_t>(*this) & static_cast<uint8_t>(other);
        }

        uint8_t get_value() const {
            ASSERT(composite.notification_enabled);
            composite.notification_enabled = false;
            uint8_t v = composite.read_raw();
            composite.notification_enabled = true;
            return v;
        }

        bool is_notification_enabled() const {
            return composite.notification_enabled;
        }

        Composite& composite;

        uint8_t data {};
    };

    using Bool = UInt8;

    uint8_t read() {
        begin_read();
        uint8_t v = read_raw();
        end_read();
        return v;
    }

    void write(uint8_t value) {
        begin_write();
        write_raw(value);
        end_write();
    }

    void enable_notification(bool enabled) {
        notification_enabled = enabled;
    }

    void suspend_notification() {
        cache.notification_enabled = notification_enabled;
        notification_enabled = false;
    }

    void restore_notification() {
        notification_enabled = cache.notification_enabled;
    }

protected:
    UInt8 make_uint8() {
        return UInt8 {*this};
    }
    Bool make_bool() {
        return Bool {*this};
    }

private:
    uint8_t read_raw() {
        return static_cast<T&>(*this).rd();
    }

    void write_raw(uint8_t value) {
        static_cast<T&>(*this).wr(value);
    }

    void begin_read() {
        suspend_notification();
    }

    void end_read() {
        if (cache.notification_enabled) {
            DebuggerMemoryWatcher::notify_read(Address, read_raw());
        }
        restore_notification();
    }

    void begin_write() {
        cache.value = read_raw();
        suspend_notification();
    }

    void end_write() {
        if (cache.notification_enabled) {
            DebuggerMemoryWatcher::notify_write(Address, cache.value, read_raw());
        }
        restore_notification();
    }

    bool notification_enabled {true};

    struct {
        uint8_t value {};
        bool notification_enabled {true};
    } cache;
};

#else

constexpr inline UInt8 make_uint8(const uint16_t address) {
    return {};
}

template <typename T, uint16_t Address>
struct Composite {
public:
    using Type = Composite<T, Address>;

    using Bool = bool;
    using UInt8 = ::UInt8;

    static constexpr UInt8 make_uint8() {
        return UInt8 {};
    }

    static constexpr Bool make_bool() {
        return Bool {};
    }

    uint8_t read() const {
        return static_cast<const T&>(*this).rd();
    }

    void write(uint8_t value) {
        static_cast<T&>(*this).wr(value);
    }
};

#endif
#endif // CELL_H