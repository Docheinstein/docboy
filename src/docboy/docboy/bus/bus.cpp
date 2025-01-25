#include "docboy/bus/bus.h"

Bus::Bus() {
    reset();
}

void Bus::load_state(Parcel& parcel) {
    requests = parcel.read_uint16();
    address = parcel.read_uint16();
    data = parcel.read_uint8();
    decay = parcel.read_uint8();
}

void Bus::save_state(Parcel& parcel) const {
    parcel.write_uint16(requests);
    parcel.write_uint16(address);
    parcel.write_uint8(data);
    parcel.write_uint8(decay);
}

void Bus::reset() {
    requests = 0;
    address = 0;
    data = 0xFF;
    decay = 0;
}
