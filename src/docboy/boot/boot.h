#ifndef BOOT_H
#define BOOT_H

#include "docboy/bootrom/macros.h"
#include "docboy/memory/byte.hpp"
#include "docboy/shared/specs.h"
#include "utils/parcel.h"

class BootIO {
public:
    void writeBOOT(uint8_t value) {
        // BOOT disable is one-way only
        BOOT |= value;
    }

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(BOOT);
    }

    void loadState(Parcel& parcel) {
        BOOT = parcel.readUInt8();
    }

    BYTE(BOOT, Specs::Registers::Boot::BOOT, IF_BOOTROM_ELSE(0b11111110, 0b11111111));
};
#endif // BOOT_H