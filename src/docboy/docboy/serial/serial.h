#ifndef SERIAL_H
#define SERIAL_H

#include "docboy/memory/byte.hpp"
#include "docboy/shared/specs.h"
#include "utils/parcel.h"

class SerialIO {
public:
    byte SB {make_byte(Specs::Registers::Serial::SB)};
    byte SC {make_byte(Specs::Registers::Serial::SC, 0b01111110)};

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(SB);
        parcel.writeUInt8(SC);
    }

    void loadState(Parcel& parcel) {
        SB = parcel.readUInt8();
        SC = parcel.readUInt8();
    }

    void writeSC(uint8_t value) {
        SC = 0b01111110 | value;
    }
};

#endif // SERIAL_H