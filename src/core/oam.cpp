#include "oam.h"

uint8_t OAM::read(uint16_t index) const {
    return Memory::read(index);
}

void OAM::write(uint16_t index, uint8_t value) {
    Memory::write(index, value);
}