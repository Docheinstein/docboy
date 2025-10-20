#include "docboy/speedswitch/speedswitch.h"

#include "utils/bits.h"
#include "utils/parcel.h"

SpeedSwitch::SpeedSwitch() {
    reset();
}

void SpeedSwitch::write_key1(uint8_t value) {
    key1.switch_speed = test_bit<Specs::Bits::SpeedSwitch::SPEED_SWITCH>(value);
}

uint8_t SpeedSwitch::read_key1() const {
    return 0x7E | key1.current_speed << Specs::Bits::SpeedSwitch::CURRENT_SPEED |
           key1.switch_speed << Specs::Bits::SpeedSwitch::SPEED_SWITCH;
}

void SpeedSwitch::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BOOL(parcel, key1.current_speed);
    PARCEL_WRITE_BOOL(parcel, key1.switch_speed);
}

void SpeedSwitch::load_state(Parcel& parcel) {
    key1.current_speed = parcel.read_bool();
    key1.switch_speed = parcel.read_bool();
}

void SpeedSwitch::reset() {
    key1.current_speed = false;
    key1.switch_speed = false;
}
