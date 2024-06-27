#include "docboy/bus/bus.h"

void Bus::load_state(Parcel& parcel) {
    requests = parcel.read_uint8();
    address = parcel.read_uint16();
}

void Bus::save_state(Parcel& parcel) const {
    parcel.write_uint8(requests);
    parcel.write_uint16(address);
}

void Bus::reset() {
    requests = 0;
    address = 0;
}
