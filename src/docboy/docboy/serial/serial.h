#ifndef SERIAL_H
#define SERIAL_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

class Serial {
public:
    void write_sc(uint8_t value);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    UInt8 sb {make_uint8(Specs::Registers::Serial::SB)};
    UInt8 sc {make_uint8(Specs::Registers::Serial::SC)};
};

#endif // SERIAL_H