#ifndef SERIAL_H
#define SERIAL_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

class SerialIO {
public:
    void save_state(Parcel& parcel) const {
        parcel.write_uint8(sb);
        parcel.write_uint8(sc);
    }

    void load_state(Parcel& parcel) {
        sb = parcel.read_uint8();
        sc = parcel.read_uint8();
    }

    void reset() {
        sb = 0;
        sc = 0b01111110;
    }

    void write_SC(uint8_t value) {
        sc = 0b01111110 | value;
    }

    UInt8 sb {make_uint8(Specs::Registers::Serial::SB)};
    UInt8 sc {make_uint8(Specs::Registers::Serial::SC)};
};

#endif // SERIAL_H