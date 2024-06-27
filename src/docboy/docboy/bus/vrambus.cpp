#include "docboy/bus/vrambus.h"

VramBus::VramBus(Vram& vram) :
    VideoBus {},
    vram {vram} {
    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memory_accessors[i] = &vram[i - Specs::MemoryLayout::VRAM::START];
    }
}
