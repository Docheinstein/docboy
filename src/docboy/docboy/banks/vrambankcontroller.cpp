#include "docboy/banks/vrambankcontroller.h"

#include "docboy/bus/vrambus.h"

uint8_t VramBankController::read_vbk() const {
    return 0xFE | vbk.bank;
}

void VramBankController::write_vbk(uint8_t value) {
    vbk.bank = test_bit<0>(value);
    vram_bus.set_vram_bank(vbk.bank);
}