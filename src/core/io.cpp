#include "io.h"
#include "memorymap.h"

uint8_t IO::read(size_t index) const {
    return Memory::read(index);
}

void IO::write(size_t index, uint8_t value) {
    if (index == MemoryMap::IO::DIV - MemoryMap::IO::START) {
        Memory::write(index, 0);
        return;
    }
    Memory::write(index, value);
}
