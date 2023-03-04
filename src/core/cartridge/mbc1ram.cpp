#include "mbc1ram.h"

MBC1RAM::MBC1RAM(const std::vector<uint8_t> &data) : MBC1(data) {

}

MBC1RAM::MBC1RAM(std::vector<uint8_t> &&data) : MBC1(data) {

}

uint8_t MBC1RAM::read(uint16_t address) const {
    if (address >= 0xA000 && address < 0xC000){
        if (!mbc.ramEnabled)
            return 0xFF;
        size_t base = 0;
        if (mbc.bankingMode == 0x1)
            base = mbc.upperRomBankSelector_ramBankSelector * 0x2000;
        size_t ramAddress = base + (address - 0xA000);
        return ram.read(ramAddress);
    }
    return MBC1::read(address);
}

void MBC1RAM::write(uint16_t address, uint8_t value) {
    if (address >= 0xA000 && address < 0xC000){
        if (!mbc.ramEnabled)
            return;
        size_t base = 0;
        if (mbc.bankingMode == 0x1)
            base = mbc.upperRomBankSelector_ramBankSelector * 0x2000;
        size_t ramAddress = base + (address - 0xA000);
        ram.write(ramAddress, value);
    } else {
        MBC1::write(address, value);
    }
}