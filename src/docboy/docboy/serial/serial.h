#ifndef SERIAL_H
#define SERIAL_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

class Serial {
public:
    uint8_t read_sc() const;
    void write_sc(uint8_t value);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    UInt8 sb {make_uint8(Specs::Registers::Serial::SB)};

    struct Sc : Composite<Specs::Registers::Serial::SC> {
        Bool transfer_enable {make_bool()};
#ifdef ENABLE_CGB
        Bool clock_speed {make_bool()};
#endif
        Bool clock_select {make_bool()};
    } sc {};
};

#endif // SERIAL_H