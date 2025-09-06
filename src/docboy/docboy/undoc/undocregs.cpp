#include "docboy/undoc/undocregs.h"

#include "utils/bits.h"

void UndocumentedRegisters::write_ff75(uint8_t value) {
    ff75 = 0x8F | keep_bits_range<6, 4>(value);
}

void UndocumentedRegisters::save_state(Parcel& parcel) const {
    parcel.write_uint8(ff72);
    parcel.write_uint8(ff73);
    parcel.write_uint8(ff74);
    parcel.write_uint8(ff75);
}

void UndocumentedRegisters::load_state(Parcel& parcel) {
    ff72 = parcel.read_uint8();
    ff73 = parcel.read_uint8();
    ff74 = parcel.read_uint8();
    ff75 = parcel.read_uint8();
}

void UndocumentedRegisters::reset() {
    ff72 = 0;
    ff73 = 0;
    ff74 = 0;
    ff75 = 0x8F;
}
