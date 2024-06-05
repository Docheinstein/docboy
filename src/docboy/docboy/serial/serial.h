#ifndef SERIAL_H
#define SERIAL_H

#include "docboy/common/specs.h"
#include "docboy/memory/byte.h"

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

    byte sb {make_byte(Specs::Registers::Serial::SB)};
    byte sc {make_byte(Specs::Registers::Serial::SC)};
};

#endif // SERIAL_H