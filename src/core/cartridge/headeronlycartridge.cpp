#include "headeronlycartridge.h"
#include <stdexcept>

HeaderOnlyCartridge::HeaderOnlyCartridge(const std::vector<uint8_t>& data) :
    Cartridge(data) {
}

HeaderOnlyCartridge::HeaderOnlyCartridge(std::vector<uint8_t>&& data) :
    Cartridge(data) {
}

uint8_t HeaderOnlyCartridge::read(uint16_t index) const {
    throw std::runtime_error("invalid operation: read");
}

void HeaderOnlyCartridge::write(uint16_t index, uint8_t value) {
    throw std::runtime_error("invalid operation: write");
}
