#include "docboy/ir/infrared.h"

#include "utils/bits.h"

void Infrared::write_rp(uint8_t value) {
    rp.read_enable = get_bits_range<Specs::Bits::Infrared::READ_ENABLE>(value);
    rp.emitting = test_bit<Specs::Bits::Infrared::EMITTING>(value);
}

uint8_t Infrared::read_rp() const {
    return 0x3C | rp.read_enable << Specs::Bits::Infrared::READ_ENABLE |
           rp.receiving << Specs::Bits::Infrared::RECEIVING | rp.emitting << Specs::Bits::Infrared::EMITTING;
}

void Infrared::save_state(Parcel& parcel) const {
    parcel.write_uint8(rp.read_enable);
    parcel.write_bool(rp.receiving);
    parcel.write_bool(rp.emitting);
}

void Infrared::load_state(Parcel& parcel) {
    rp.read_enable = parcel.read_uint8();
    rp.receiving = parcel.read_uint8();
    rp.emitting = parcel.read_uint8();
}

void Infrared::reset() {
    rp.read_enable = 0;
    rp.receiving = true;
    rp.emitting = false;
}
