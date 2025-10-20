#include "docboy/banks/vrambankcontroller.h"

#include "docboy/bus/vrambus.h"

uint8_t VramBankController::read_vbk() const {
    return 0xFE | vbk.bank;
}

void VramBankController::write_vbk(uint8_t value) {
    vbk.bank = test_bit<0>(value);
    vram_bus.set_vram_bank(vbk.bank);
}

void VramBankController::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BOOL(parcel, vbk.bank);
}

void VramBankController::load_state(Parcel& parcel) {
    vbk.bank = parcel.read_bool();
}

void VramBankController::reset() {
    write_vbk(0);
}
