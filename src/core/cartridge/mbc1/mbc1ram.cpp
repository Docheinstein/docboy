#include "mbc1ram.h"
#include "utils/arrayutils.h"

MBC1RAM::MBC1RAM(const std::vector<uint8_t>& data) :
    MBC1(data),
    ram() {
}

MBC1RAM::MBC1RAM(std::vector<uint8_t>&& data) :
    MBC1(data),
    ram() {
}

void MBC1RAM::loadState(IReadableState& state) {
    MBC1::loadState(state);
    state.readBytes(ram);
}

void MBC1RAM::saveState(IWritableState& state) {
    MBC1::saveState(state);
    state.writeBytes(ram, array_size(ram));
}

uint8_t MBC1RAM::read(uint16_t address) const {
    if (address >= 0xA000 && address < 0xC000) {
        if (!ramEnabled)
            return 0xFF;
        size_t base = 0;
        if (bankingMode == 0x1)
            base = upperRomBankSelector_ramBankSelector * 0x2000;
        size_t ramAddress = base + (address - 0xA000);
        return ram[ramAddress];
    }
    return MBC1::read(address);
}

void MBC1RAM::write(uint16_t address, uint8_t value) {
    if (address >= 0xA000 && address < 0xC000) {
        if (!ramEnabled)
            return;
        size_t base = 0;
        if (bankingMode == 0x1)
            base = upperRomBankSelector_ramBankSelector * 0x2000;
        size_t ramAddress = base + (address - 0xA000);
        ram[ramAddress] = value;
    } else {
        MBC1::write(address, value);
    }
}