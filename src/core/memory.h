#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>
#include "log/log.h"
#include <cstring>

class IMemory {
public:
    virtual ~IMemory();

    [[nodiscard]] virtual uint8_t read(size_t index) const = 0;
    virtual void write(size_t index, uint8_t value) = 0;
    virtual void reset() = 0;
};

template<size_t n>
class Memory : public IMemory {
public:
    Memory() : memory() {

    }

    [[nodiscard]] uint8_t read(size_t index) const override {
        return memory[index];
    }

    void write(size_t index, uint8_t value) override {
        memory[index] = value;
    }

    void reset() override {
        memset(memory, 0, 4096);
    }

private:
    uint8_t memory[n];
};


#endif // MEMORY_H