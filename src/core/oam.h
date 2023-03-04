#ifndef OAM_H
#define OAM_H

#include "memory.h"
#include "definitions.h"

class OAM : public Memory<MemoryMap::OAM::SIZE> {
    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;
};

#endif // OAM_H