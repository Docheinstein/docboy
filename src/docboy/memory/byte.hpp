#ifndef BYTE_HPP
#define BYTE_HPP

#include "fwd/bytefwd.h"

#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
#include "utils/bits.hpp"

class byte {
public:
    byte() = default;
    explicit byte(uint16_t address, uint8_t initialValue = 0);

    byte(const byte& b) = delete;
    byte(byte&& b) = delete;
    byte& operator=(const byte& b) = delete;
    byte& operator=(byte&& b) = delete;

    void setMemoryAddress(uint16_t addr);

    byte& operator=(uint8_t value);
    operator uint8_t() const;

    byte& operator++();
    byte& operator|=(uint64_t value);
    byte& operator&=(uint64_t value);
    uint8_t operator|(uint64_t value) const;
    uint8_t operator&(uint64_t value) const;
    uint8_t operator|(const byte& other) const;
    uint8_t operator&(const byte& other) const;

private:
    static constexpr uint32_t INVALID_MEMORY_ADDRESS = UINT32_MAX;

    uint32_t address {INVALID_MEMORY_ADDRESS};
    uint8_t data {};
};

#endif
#endif // BYTE_HPP