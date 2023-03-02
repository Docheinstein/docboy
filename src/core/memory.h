#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>
#include "utils/log.h"
#include <cstring>

class IMemory {
public:
    virtual ~IMemory() = default;

    [[nodiscard]] virtual uint8_t read(uint16_t index) const = 0;
    virtual void write(uint16_t index, uint8_t value) = 0;
};

template<size_t n>
class Memory : public IMemory {
public:
    Memory() : memory() {}

    [[nodiscard]] uint8_t read(uint16_t index) const override {
        return memory[index];
    }

    void write(uint16_t index, uint8_t value) override {
        memory[index] = value;
    }

private:
    uint8_t memory[n];
};


#endif // MEMORY_H