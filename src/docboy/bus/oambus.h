#ifndef OAMBUS_H
#define OAMBUS_H

#include "acqbus.h"
#include "docboy/memory/oam.h"

class OamBus : public AcquirableBus<OamBus> {
    friend class Bus<OamBus>;

public:
    explicit OamBus(Oam& oam);

    const byte& operator[](uint16_t index) const {
        return oam[index];
    }

    byte& operator[](uint16_t index) {
        return oam[index];
    }

private:
    [[nodiscard]] uint8_t readOam(uint16_t address) const;
    void writeOam(uint16_t address, uint8_t value);

    Oam& oam;
};

#endif // OAMBUS_H