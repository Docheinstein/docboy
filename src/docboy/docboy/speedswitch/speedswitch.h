#ifndef SPEEDSWITCH_H
#define SPEEDSWITCH_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

class Parcel;

class SpeedSwitch {
public:
    SpeedSwitch();

    void write_key1(uint8_t value);
    uint8_t read_key1() const;

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    struct Stat : Composite<Specs::Registers::SpeedSwitch::KEY1> {
        Bool current_speed {make_bool()};
        Bool switch_speed {make_bool()};
    } key1 {};
};

#endif // SPEEDSWITCH_H
