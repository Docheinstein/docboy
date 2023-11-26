#include "vrambus.h"

VramBus::VramBus(Vram& vram) :
    AcquirableBus<VramBus>(),
    vram(vram) {
    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memoryAccessors[i] = {&VramBus::readVram, &VramBus::writeVram};
    }
}

uint8_t VramBus::readVram(uint16_t address) const {
    if (!isAcquired())
        return vram[address - Specs::MemoryLayout::VRAM::START];
    return 0xFF;
}

void VramBus::writeVram(uint16_t address, uint8_t value) {
    if (!isAcquired())
        vram[address - Specs::MemoryLayout::VRAM::START] = value;
}
