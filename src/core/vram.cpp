#include "vram.h"

uint8_t VRAM::read(uint16_t index) const {
    return Memory::read(index);
//    return 0xFF;
}

void VRAM::write(uint16_t index, uint8_t value) {
    Memory::write(index, value);
}
