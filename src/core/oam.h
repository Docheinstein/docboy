#ifndef OAM_H
#define OAM_H

#include "memory.h"
#include "definitions.h"
#include "components.h"

class OAM : public MemoryImpl {
public:
    OAM();

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;
};

#endif // OAM_H