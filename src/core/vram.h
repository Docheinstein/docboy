#ifndef VRAM_H
#define VRAM_H

#include "memory.h"
#include "definitions.h"

class VRAM : public Memory<MemoryMap::IO::SIZE> {
    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;
};

#endif // VRAM_H