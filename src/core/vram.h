#ifndef VRAM_H
#define VRAM_H

#include "memory.h"
#include "definitions.h"
#include "components.h"

class VRAM : public MemoryImpl {
public:
    VRAM();
    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;
};

#endif // VRAM_H