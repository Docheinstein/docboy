#include "mbc1.h"
#include "utils/binutils.h"

MBC1::MBC1(const std::vector<uint8_t>& data) :
    Cartridge(data),
    mbc() {
}

MBC1::MBC1(std::vector<uint8_t>&& data) :
    Cartridge(data),
    mbc() {
}

uint8_t MBC1::read(uint16_t address) const {
    /*
     *  Bits: 20 19 18 17 16 15 14 13 12 .. 01 00
     *        \___/ \____________/ \____________/
     *          |          |            \----------- From Game Boy address
     *          |          \------------------------ As 2000–3FFF bank register
     *          \----------------------------------- As 4000–5FFF bank register
     */
    if (address < 0x4000) {
        return rom[address];
    }
    if (address < 0x8000) {
        uint16_t bankAddress = address - 0x4000;
        uint8_t bankSelector = mbc.romBankSelector > 0 ? mbc.romBankSelector : 0x1;
        if (mbc.bankingMode == 0x0)
            bankSelector = bankSelector | (mbc.upperRomBankSelector_ramBankSelector << 5);
        size_t romAddress = (bankSelector << 14) + bankAddress;

        return rom[romAddress];
    }
    throw ReadMemoryException("Read at address " + hex(address) + " is not allowed");
}

void MBC1::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        mbc.ramEnabled = value == 0xA;
    } else if (address < 0x4000) {
        mbc.romBankSelector = keep_bits<5>(value > 0 ? value : 0x1);
    } else if (address < 0x6000) {
        mbc.upperRomBankSelector_ramBankSelector = keep_bits<2>(value);
    } else if (address < 0x8000) {
        mbc.bankingMode = keep_bits<1>(value);
    } else {
        throw WriteMemoryException("Write at address " + hex(address) + " is not allowed");
    }
}
