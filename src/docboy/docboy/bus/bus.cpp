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
    PARCEL_WRITE_UINT16(parcel, requests);
    PARCEL_WRITE_UINT16(parcel, address);
    PARCEL_WRITE_UINT8(parcel, data);
    PARCEL_WRITE_UINT8(parcel, decay);
}

void Bus::reset() {
    requests = 0;
    address = 0;
    data = 0xFF;
    decay = 0;
}
