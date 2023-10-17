#include "lcd.h"
#include "utils/parcel.h"

void Lcd::saveState(Parcel& parcel) const {
    parcel.writeBytes(pixels, sizeof(pixels));
    parcel.writeUInt16(cursor);
    IF_DEBUGGER(parcel.writeUInt8(x));
    IF_DEBUGGER(parcel.writeUInt8(y));
}

void Lcd::loadState(Parcel& parcel) {
    parcel.readBytes(pixels, sizeof(pixels));
    cursor = parcel.readUInt16();
    IF_DEBUGGER(x = parcel.readUInt8());
    IF_DEBUGGER(y = parcel.readUInt8());
}