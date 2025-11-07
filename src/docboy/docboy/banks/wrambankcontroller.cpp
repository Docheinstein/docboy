#include "docboy/banks/wrambankcontroller.h"

#include "docboy/bus/wrambus.h"

uint8_t WramBankController::read_svbk() const {
    return 0xF8 | svbk.bank << Specs::Bits::WramBank::BANK;
}

void WramBankController::write_svbk(uint8_t value) {
    svbk.bank = get_bits_range<Specs::Bits::WramBank::BANK>(value);

    // Note: both SVBK=0 and SVBK=1 are mapped to the same WRAM2 bank
    const uint8_t wram2_bank = svbk.bank > 0 ? svbk.bank - 1 : 0;

    wram_bus.set_wram2_bank(wram2_bank);
}

void WramBankController::save_state(Parcel& parcel) const {
    PARCEL_WRITE_UINT8(parcel, svbk.bank);
}

void WramBankController::load_state(Parcel& parcel) {
    svbk.bank = parcel.read_uint8();
}

void WramBankController::reset() {
    write_svbk(0);
}
