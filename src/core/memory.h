#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>

class IMemory {
public:
    virtual ~IMemory();
    virtual uint8_t & operator[](size_t index) = 0;
    virtual const uint8_t & operator[](size_t index) const = 0;
};

class Memory : public IMemory {
public:
    Memory();

    uint8_t & operator[](size_t index) override;
    const uint8_t & operator[](size_t index) const override;

private:
    uint8_t memory[4096];
};

#endif // MEMORY_H