#ifndef IO_H
#define IO_H

#include "memory.h"

class IO : public Memory<256> {
    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;
};

#endif // IO_H