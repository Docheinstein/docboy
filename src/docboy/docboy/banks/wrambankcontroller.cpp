#include "docboy/banks/wrambankcontroller.h"

#include "docboy/bus/extbus.h"

uint8_t WramBankController::read_svbk() const {
    return 0xF8 | svbk.bank;
}

void WramBankController::write_svbk(uint8_t value) {
    svbk.bank = keep_bits<3>(value);
    ext_bus.set_wram2_bank(svbk.bank);
}