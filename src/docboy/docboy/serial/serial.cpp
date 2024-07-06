#include "docboy/serial/serial.h"

void Serial::write_sc(uint8_t value) {
    sc = 0b01111110 | value;
}

void Serial::save_state(Parcel& parcel) const {
    parcel.write_uint8(sb);
    parcel.write_uint8(sc);
}

void Serial::load_state(Parcel& parcel) {
    sb = parcel.read_uint8();
    sc = parcel.read_uint8();
}

void Serial::reset() {
    sb = 0;
    sc = 0b01111110;
}
