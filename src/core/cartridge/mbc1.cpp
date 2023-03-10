#include "mbc1.h"
#include "utils/binutils.h"

MBC1::MBC1(const std::vector <uint8_t> &data) :
    Cartridge(data), mbc() {

}

MBC1::MBC1(std::vector<uint8_t> &&data) :
    Cartridge(data), mbc() {

}

uint8_t MBC1::read(uint16_t address) const {
    if (address < 0x4000) {
        return memory[address];
    }
    if (address < 0x8000) {
        size_t base = 0;
        if (mbc.bankingMode == 0x0)
            base = mbc.upperRomBankSelector_ramBankSelector * 0x80000;
        size_t romAddress = base + mbc.romBankSelector * 0x4000 + address;
        return memory[romAddress];
    }
    throw std::runtime_error("Read at address " + hex(address) + " is not allowed");
}

void MBC1::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        mbc.ramEnabled = value == 0xA;
    } else if (address < 0x4000) {
        mbc.romBankSelector = bitmasked<5>(value > 0 ? value : 0x1);
    } else if (address < 0x6000) {
        mbc.upperRomBankSelector_ramBankSelector = bitmasked<2>(value);
    } else if (address < 0x8000) {
        mbc.bankingMode = bitmasked<1>(value);
    } else {
        throw std::runtime_error("Write at address " + hex(address) + " is not allowed");
    }
}
