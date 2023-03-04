#include "bootrom.h"

BootROM::BootROM(const std::vector<uint8_t> &data) {
    this->rom = data;
}

BootROM::BootROM(std::vector<uint8_t> &&data) {
    this->rom = std::move(data);
}

uint8_t BootROM::read(uint16_t index) const {
    return rom[index];
}
