#ifndef MEMORY_H
#define MEMORY_H

#include "readable.h"
#include "writable.h"

class IMemory : public IReadableMemory, public IWritableMemory {};


class Memory : public IMemory {
public:
    explicit Memory(size_t size);
    explicit Memory(const std::vector<uint8_t> &data);
    explicit Memory(std::vector<uint8_t> &&data);

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

protected:
    std::vector<uint8_t> memory;
};

#endif // MEMORY_H