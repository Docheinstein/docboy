#include "mbc1ram.h"

MBC1RAM::MBC1RAM(const std::vector<uint8_t>& data) :
    MBC1(data),
    ram() {
}

MBC1RAM::MBC1RAM(std::vector<uint8_t>&& data) :
    MBC1(data),
    ram() {
}

uint8_t MBC1RAM::read(uint16_t address) const {
    if (address >= 0xA000 && address < 0xC000) {
        if (!this->mbc.ramEnabled)
            return 0xFF;
        size_t base = 0;
        if (this->mbc.bankingMode == 0x1)
            base = this->mbc.upperRomBankSelector_ramBankSelector * 0x2000;
        size_t ramAddress = base + (address - 0xA000);
        return ram[ramAddress];
    }
    return MBC1::read(address);
}

void MBC1RAM::write(uint16_t address, uint8_t value) {
    if (address >= 0xA000 && address < 0xC000) {
        if (!this->mbc.ramEnabled)
            return;
        size_t base = 0;
        if (this->mbc.bankingMode == 0x1)
            base = this->mbc.upperRomBankSelector_ramBankSelector * 0x2000;
        size_t ramAddress = base + (address - 0xA000);
        ram[ramAddress] = value;
    } else {
        MBC1::write(address, value);
    }
}