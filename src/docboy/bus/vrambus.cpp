#include "vrambus.h"

VramBus::VramBus(Vram& vram) :
    VideoBus(),
    vram(vram) {
    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memoryAccessors[i] = {&VramBus::readVram, &VramBus::writeVram};
    }
}

uint8_t VramBus::readVram(uint16_t address) const {
    return vram[address - Specs::MemoryLayout::VRAM::START];
}

void VramBus::writeVram(uint16_t address, uint8_t value) {
    vram[address - Specs::MemoryLayout::VRAM::START] = value;
}
