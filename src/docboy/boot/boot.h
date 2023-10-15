#ifndef BOOT_H
#define BOOT_H

#include "docboy/memory/byte.hpp"
#include "docboy/shared/specs.h"
#include "utils/parcel.h"

class BootIO {
public:
#ifdef ENABLE_BOOTROM
    explicit BootIO(bool bootRomEnabled) {
        BOOT = bootRomEnabled ? 0 : 1;
    }
#endif

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(BOOT);
    }

    void loadState(Parcel& parcel) {
        BOOT = parcel.readUInt8();
    }

    BYTE(BOOT, Specs::Registers::Boot::BOOT);
};
#endif // BOOT_H