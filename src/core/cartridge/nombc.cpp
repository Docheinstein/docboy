#include "nombc.h"

uint8_t NoMBCCartridge::read(uint16_t address) const {
    return rom[address];
}

void NoMBCCartridge::write(uint16_t address, uint8_t value) {
    rom[address] = value;
}

NoMBCCartridge::NoMBCCartridge(const std::vector<uint8_t> &data) : Cartridge(data) {

}

NoMBCCartridge::NoMBCCartridge(std::vector<uint8_t> &&data) : Cartridge(data) {

}
