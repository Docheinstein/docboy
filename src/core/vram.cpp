#include "vram.h"

VRAM::VRAM() : MemoryImpl(MemoryMap::VRAM::SIZE) {

}

uint8_t VRAM::read(uint16_t index) const {
    return MemoryImpl::read(index);
}

void VRAM::write(uint16_t index, uint8_t value) {
    MemoryImpl::write(index, value);
}
