#include "docboy/boot/boot.h"

#ifdef ENABLE_BOOTROM
#include "docboy/mmu/mmu.h"
#endif

void Boot::write_boot(uint8_t value) {
    // BOOT disable is one-way only
#ifdef ENABLE_BOOTROM
    if (!test_bit<Specs::Bits::Boot::BOOT_ENABLE>(boot) && test_bit<Specs::Bits::Boot::BOOT_ENABLE>(value)) {
        mmu.lock_boot_rom();
    }
#endif
    boot |= value;
}

void Boot::save_state(Parcel& parcel) const {
    parcel.write_uint8(boot);
}

void Boot::load_state(Parcel& parcel) {
    boot = parcel.read_uint8();
}

void Boot::reset() {
    boot = if_bootrom_else(0b11111110, 0b11111111);
}
