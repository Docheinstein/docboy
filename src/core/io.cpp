#include "io.h"
#include "definitions.h"

uint8_t IO::read(uint16_t index) const {
    return Memory::read(index);
}

void IO::write(uint16_t index, uint8_t value) {
    if (MemoryMap::IO::START + index == MemoryMap::IO::DIV) {
        Memory::write(index, 0);
        return;
    }
    Memory::write(index, value);
}
