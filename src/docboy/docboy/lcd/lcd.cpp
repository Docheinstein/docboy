#include "docboy/lcd/lcd.h"
#include "docboy/lcd/colormap.h"

#include "utils/parcel.h"

void Lcd::save_state(Parcel& parcel) const {
    parcel.write_bytes(pixels, sizeof(pixels));
    parcel.write_uint16(cursor);
#ifdef ENABLE_DEBUGGER
    parcel.write_uint8(x);
    parcel.write_uint8(y);
#endif
}

void Lcd::load_state(Parcel& parcel) {
    parcel.read_bytes(pixels, sizeof(pixels));
    cursor = parcel.read_uint16();
#ifdef ENABLE_DEBUGGER
    x = parcel.read_uint8();
    y = parcel.read_uint8();
#endif
}

Lcd::Lcd() {
    palette.assign(DEFAULT_PALETTE.begin(), DEFAULT_PALETTE.end());
}
