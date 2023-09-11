#include "mbc1.h"
#include "utils/binutils.h"
#include "utils/exceptionutils.h"

MBC1::MBC1(const std::vector<uint8_t>& data) :
    Cartridge(data) {
}

MBC1::MBC1(std::vector<uint8_t>&& data) :
    Cartridge(data) {
}

void MBC1::loadState(IReadableState& state) {
    ramEnabled = state.readBool();
    romBankSelector = state.readUInt8();
    upperRomBankSelector_ramBankSelector = state.readUInt8();
    bankingMode = state.readUInt8();
}

void MBC1::saveState(IWritableState& state) {
    state.writeBool(ramEnabled);
    state.writeUInt8(romBankSelector);
    state.writeUInt8(upperRomBankSelector_ramBankSelector);
    state.writeUInt8(bankingMode);
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
        uint8_t bankSelector = romBankSelector > 0 ? romBankSelector : 0x1;
        if (bankingMode == 0x0)
            bankSelector = bankSelector | (upperRomBankSelector_ramBankSelector << 5);
        size_t romAddress = (bankSelector << 14) + bankAddress;

        return rom[romAddress];
    }
    THROW(ReadMemoryException("Read at address " + hex(address) + " is not allowed"));
}

void MBC1::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        ramEnabled = value == 0xA;
    } else if (address < 0x4000) {
        romBankSelector = keep_bits<5>(value > 0 ? value : 0x1);
    } else if (address < 0x6000) {
        upperRomBankSelector_ramBankSelector = keep_bits<2>(value);
    } else if (address < 0x8000) {
        bankingMode = keep_bits<1>(value);
    } else {
        THROW(WriteMemoryException("Write at address " + hex(address) + " is not allowed"));
    }
}