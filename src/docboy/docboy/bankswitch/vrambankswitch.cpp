#include "docboy/bankswitch/vrambankswitch.h"

uint8_t VramBankSwitch::read_vbk() const {
    return 0xFE | vbk.bank;
}

void VramBankSwitch::write_vbk(uint8_t value) {
    vbk.bank = test_bit<0>(value);
    vram_bus.switch_bank(vbk.bank);
}