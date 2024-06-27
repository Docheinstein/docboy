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
