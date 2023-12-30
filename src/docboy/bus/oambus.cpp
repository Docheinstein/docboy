#include "oambus.h"

OamBus::OamBus(Oam& oam) :
    VideoBus(),
    oam(oam) {
    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memoryAccessors[i] = {&OamBus::readOam, &OamBus::writeOam};
    }
}

uint8_t OamBus::readOam(uint16_t address) const {
    return oam[address - Specs::MemoryLayout::OAM::START];
}

void OamBus::writeOam(uint16_t address, uint8_t value) {
    oam[address - Specs::MemoryLayout::OAM::START] = value;
}