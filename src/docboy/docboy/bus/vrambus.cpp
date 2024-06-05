#include "docboy/bus/vrambus.h"

VramBus::VramBus(Vram& vram) :
    VideoBus {},
    vram {vram} {
    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memory_accessors[i] = {&VramBus::read_vram, &VramBus::write_vram};
    }
}

uint8_t VramBus::read_vram(uint16_t address) const {
    return vram[address - Specs::MemoryLayout::VRAM::START];
}

void VramBus::write_vram(uint16_t address, uint8_t value) {
    vram[address - Specs::MemoryLayout::VRAM::START] = value;
}
