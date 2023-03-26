#include "nombc.h"
#include "utils/log.h"
#include "utils/binutils.h"

NoMBC::NoMBC(const std::vector<uint8_t> &data) : Cartridge(data) {

}

NoMBC::NoMBC(std::vector<uint8_t> &&data) : Cartridge(data) {

}

uint8_t NoMBC::read(uint16_t address) const {
    return rom[address];
}

void NoMBC::write(uint16_t address, uint8_t value) {
    WARN() << "Write at address " + hex(address) + " is ignored (NoMBC cartridge)" << std::endl;
}