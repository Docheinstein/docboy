#include "nombc.h"

uint8_t NoMBC::read(uint16_t address) const {
    return rom[address];
}

void NoMBC::write(uint16_t address, uint8_t value) {
    rom[address] = value;
}

NoMBC::NoMBC(const std::vector<uint8_t> &data) : Cartridge(data) {

}

NoMBC::NoMBC(std::vector<uint8_t> &&data) : Cartridge(data) {

}
