#include "mbc3ram.h"
#include "utils/arrayutils.h"

MBC3RAM::MBC3RAM(const std::vector<uint8_t>& data) :
    MBC3(data) {
}

MBC3RAM::MBC3RAM(std::vector<uint8_t>&& data) :
    MBC3(data) {
}

void MBC3RAM::loadState(IReadableState& state) {
    MBC3::loadState(state);
    state.readBytes(ram);
}

void MBC3RAM::saveState(IWritableState& state) {
    MBC3::saveState(state);
    state.writeBytes(ram, array_size(ram));
}

uint8_t MBC3RAM::read(uint16_t address) const {
    if (address >= 0xA000 && address < 0xC000) {
        if (ramBankSelector_rtcRegisterSelector < 0x4) {
            return ram[ramBankSelector_rtcRegisterSelector * 0x2000 + (address - 0xA000)];
        }
    }

    return MBC3::read(address);
}

void MBC3RAM::write(uint16_t address, uint8_t value) {
    if (address >= 0xA000 && address < 0xC000) {
        if (ramBankSelector_rtcRegisterSelector < 0x4) {
            ram[ramBankSelector_rtcRegisterSelector * 0x2000 + (address - 0xA000)] = value;
            return;
        }
    }

    MBC3::write(address, value);
}
