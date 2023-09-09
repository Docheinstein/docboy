#include "cartridge.h"
#include <cstring>
#include <map>

Cartridge::Cartridge(const std::vector<uint8_t>& data) :
    rom(data) {
}

Cartridge::Cartridge(std::vector<uint8_t>&& data) :
    rom(data) {
}
