#ifndef BOOT_H
#define BOOT_H

#include "docboy/bootrom/macros.h"
#include "docboy/memory/byte.hpp"
#include "docboy/shared/specs.h"
#include "lock.h"
#include "utils/parcel.h"

class BootIO {
public:
#ifdef ENABLE_BOOTROM
    explicit BootIO(BootLock bootLock) :
        bootLock(bootLock) {
    }
#endif

    void writeBOOT(uint8_t value) {
        // BOOT disable is one-way only
#ifdef ENABLE_BOOTROM
        if (!test_bit<Specs::Bits::Boot::BOOT_ENABLE>(BOOT) && test_bit<Specs::Bits::Boot::BOOT_ENABLE>(value))
            bootLock.lock();
#endif
        BOOT |= value;
    }

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(BOOT);
    }

    void loadState(Parcel& parcel) {
        BOOT = parcel.readUInt8();
    }

    byte BOOT {make_byte(Specs::Registers::Boot::BOOT, IF_BOOTROM_ELSE(0b11111110, 0b11111111))};

private:
    IF_BOOTROM(BootLock bootLock);
};

#endif // BOOT_H