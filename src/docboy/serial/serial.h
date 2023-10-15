#ifndef SERIAL_H
#define SERIAL_H

#include "docboy/memory/byte.hpp"
#include "utils/parcel.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/shared/specs.h"
#endif

class SerialIO {
public:
    BYTE(SB, Specs::Registers::Serial::SB);
    BYTE(SC, Specs::Registers::Serial::SC);

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(SB);
        parcel.writeUInt8(SC);
    }

    void loadState(Parcel& parcel) {
        SB = parcel.readUInt8();
        SC = parcel.readUInt8();
    }
};

#endif // SERIAL_H