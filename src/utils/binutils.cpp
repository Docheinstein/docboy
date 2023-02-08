#include "binutils.h"
#include <bitset>

void bin(uint8_t value, std::ostream &os) {
    os << std::bitset<8>(value);
}

void bin(uint16_t value, std::ostream &os) {
    os << std::bitset<16>(value);
}

void hex(uint8_t value, std::ostream &os) {
    os << std::hex << std::setfill('0') << std::setw(2) << (uint16_t) value;
}

void hex(uint16_t value, std::ostream &os) {
    os << std::hex << std::setfill('0') << std::setw(4) << (uint16_t) value;
}

uint16_t concat_bytes(uint8_t msb, uint8_t lsb) {
    return ((uint16_t) msb) << 8 | lsb;
}