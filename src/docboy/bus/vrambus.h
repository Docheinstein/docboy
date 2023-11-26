#ifndef VRAMBUS_H
#define VRAMBUS_H

#include "acqbus.h"
#include "docboy/memory/vram.h"

class VramBus : public AcquirableBus<VramBus> {
    friend class Bus<VramBus>;

public:
    explicit VramBus(Vram& vram);

    const byte& operator[](uint16_t index) const {
        return vram[index];
    }

    byte& operator[](uint16_t index) {
        return vram[index];
    }

private:
    [[nodiscard]] uint8_t readVram(uint16_t address) const;
    void writeVram(uint16_t address, uint8_t value);

    Vram& vram;
};

#endif // VRAMBUS_H