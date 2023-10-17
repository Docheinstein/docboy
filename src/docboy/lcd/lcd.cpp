#include "lcd.h"
#include "utils/parcel.h"

void Lcd::saveState(Parcel& parcel) const {
    parcel.writeBytes(pixels, sizeof(pixels));
    parcel.writeUInt16(cursor);
    DEBUGGER_ONLY(parcel.writeUInt8(x));
    DEBUGGER_ONLY(parcel.writeUInt8(y));
}

void Lcd::loadState(Parcel& parcel) {
    parcel.readBytes(pixels, sizeof(pixels));
    cursor = parcel.readUInt16();
    DEBUGGER_ONLY(x = parcel.readUInt8());
    DEBUGGER_ONLY(y = parcel.readUInt8());
}