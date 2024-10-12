#ifndef BOOT_H
#define BOOT_H

#include "docboy/bootrom/helpers.h"
#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/bits.h"
#include "utils/parcel.h"

#ifdef ENABLE_BOOTROM
class Mmu;
#endif

class Boot {
public:
#ifdef ENABLE_BOOTROM
    explicit Boot(Mmu& mmu) :
        mmu {mmu} {
    }
#endif

    void write_boot(uint8_t value);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    UInt8 boot {make_uint8(Specs::Registers::Boot::BOOT)};

private:
#ifdef ENABLE_BOOTROM
    Mmu& mmu;
#endif
};

#endif // BOOT_H