#include "docboy/mode/operatingmode.h"

#ifndef ENABLE_BOOTROM
#include "docboy/cartridge/header.h"
#endif

#include "utils/bits.h"
#include "utils/parcel.h"

#ifdef ENABLE_BOOTROM
OperatingMode::OperatingMode()
#else
OperatingMode::OperatingMode(CartridgeHeader& header) :
    header {header}
#endif
{
    reset();
}

void OperatingMode::write_key0(uint8_t value) {
    // KEY0 can be written only by the bootrom; it is unmapped (along with the bootrom) when 0xFF50 is finally written.
    // The canonical cgb_boot bootrom either:
    // - Copies the CGB flag (cartridge byte 0x143) to KEY0 (with the intention of enabling CGB operating mode)
    // - Write 0x04 to KEY0 (with the intention of enabling DMG mode)
    //
    // Therefore, bit 2 is used to enable DMG mode.
    //
    // But actually, things get more complex: if the CGB flag (0x143) has bit 2 or bit 3 set,
    // DMG mode is entered as well!

    // Indeed, bit 3 allow to enter the so called PGB modes, in which LCD is operated by an external signal.
    // In PGB mode (sometimes called DMG ext mode), some functionalities of CGB are disabled as in DMG mode,
    // but others continue to work as in CGB mode (see CpuBus for the list of functionalities enabled in each mode).
    //
    // Note that in DMG ext mode, the palettes are not loaded as in DMG mode (and neither they can't be written to,
    // because BCPD is locked as well), so the hardware enters an irreversible blank-palettes state.
    //
    // Bit 3 has priority over bit 2: it means that if both bits are set (e.g. CGB flag equals to 0x8F),
    // DMG ext mode is entered.
    //
    key0.dmg_ext_mode = get_bit<Specs::Bits::OperatingMode::DMG_EXT_MODE>(value);
    key0.dmg_mode = get_bit<Specs::Bits::OperatingMode::DMG_MODE>(value);

    // Cached for convenience.
    cgb_mode = !key0.dmg_mode && !key0.dmg_ext_mode;
}

uint8_t OperatingMode::read_key0() const {
    // TODO: this has not been verified, as the CGB boot rom does not read KEY0.
    return key0.dmg_ext_mode << Specs::Bits::OperatingMode::DMG_EXT_MODE | key0.dmg_mode
                                                                               << Specs::Bits::OperatingMode::DMG_MODE;
}

void OperatingMode::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BOOL(parcel, key0.dmg_ext_mode);
    PARCEL_WRITE_BOOL(parcel, key0.dmg_mode);
    PARCEL_WRITE_BOOL(parcel, cgb_mode);
}

void OperatingMode::load_state(Parcel& parcel) {
    key0.dmg_ext_mode = parcel.read_bool();
    key0.dmg_mode = parcel.read_bool();
    cgb_mode = parcel.read_bool();
}

void OperatingMode::reset() {
#ifdef ENABLE_BOOTROM
    key0.dmg_ext_mode = false;
    key0.dmg_mode = false;
#else
    // Deduce operating mode from header.
    // In particular, cgb_boot behaves as follows:
    // - If bit 7 of CGB flag (0x143) is set, copy CGB flag to KEY0 (usually leads to CGB mode, but can be anything
    // actually).
    // - Otherwise, write 0x04 to KEY0, that is DMG mode.
    uint8_t cgb_flag = CartridgeHeaderHelpers::cgb_flag(header);
    if (test_bit<Specs::Bits::Cartridge::CgbFlag::CGB_GAME>(cgb_flag)) {
        key0.dmg_ext_mode = get_bit<Specs::Bits::OperatingMode::DMG_EXT_MODE>(cgb_flag);
        key0.dmg_mode = get_bit<Specs::Bits::OperatingMode::DMG_MODE>(cgb_flag);
    } else {
        key0.dmg_ext_mode = false;
        key0.dmg_mode = true;
    }
#endif

    cgb_mode = !key0.dmg_mode && !key0.dmg_ext_mode;
}
