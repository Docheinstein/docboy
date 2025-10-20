#include "docboy/lcd/lcd.h"
#include "docboy/lcd/colormap.h"

#include "utils/parcel.h"

Lcd::Lcd() {
    appearance = DEFAULT_APPEARANCE;
    reset();
}

void Lcd::set_appearance(const Appearance& a) {
    appearance = a;
}

void Lcd::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BYTES(parcel, pixels, sizeof(pixels));
    PARCEL_WRITE_UINT16(parcel, cursor);
#ifdef ENABLE_DEBUGGER
    PARCEL_WRITE_UINT8(parcel, x);
    PARCEL_WRITE_UINT8(parcel, y);
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

void Lcd::reset() {
    memset(pixels, 0, sizeof(pixels));
    cursor = 0;
#ifdef ENABLE_DEBUGGER
    x = 0;
    y = 0;
#endif
}