#ifndef BYTE_HPP
#define BYTE_HPP

#include "fwd/bytefwd.h"

#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
struct byte {
    static constexpr uint32_t INVALID_MEMORY_ADDRESS = UINT32_MAX;

    byte() = default;
    constexpr explicit byte(uint16_t address) :
        address {address} {
    }

    byte(const byte& b) = delete;
    byte(byte&& b) = delete;
    byte& operator=(const byte& b) = delete;
    byte& operator=(byte&& b) = delete;

    byte& operator=(uint8_t value);
    operator uint8_t() const;

    byte& operator++();
    byte& operator+=(uint64_t value);
    byte& operator--();
    byte& operator-=(uint64_t value);
    byte& operator|=(uint64_t value);
    byte& operator&=(uint64_t value);
    uint8_t operator|(uint64_t value) const;
    uint8_t operator&(uint64_t value) const;
    uint8_t operator|(const byte& other) const;
    uint8_t operator&(const byte& other) const;

    uint32_t address {INVALID_MEMORY_ADDRESS};
    uint8_t data {};
};

constexpr inline byte make_byte(const uint16_t address) {
    return byte {address};
}

#else

constexpr inline byte make_byte(const uint16_t address) {
    return {};
}

#endif
#endif // BYTE_HPP