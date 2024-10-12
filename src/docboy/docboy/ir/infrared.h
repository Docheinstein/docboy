#ifndef INFRARED_H
#define INFRARED_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

class Infrared {
public:
    void write_rp(uint8_t value);
    uint8_t read_rp() const;

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    struct Rp : Composite<Specs::Registers::Infrared::RP> {
        UInt8 read_enable {make_uint8()};
        Bool receiving {make_bool()};
        Bool emitting {make_bool()};
    } rp {};
};

#endif // INFRARED_H
