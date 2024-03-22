#include "oambus.h"

OamBus::OamBus(Oam& oam) :
    VideoBus(),
    oam(oam) {

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memoryAccessors[i] = {&OamBus::readOam, &OamBus::writeOam};
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memoryAccessors[i] = {&OamBus::readFF, &OamBus::writeNop};
    }
}

uint8_t OamBus::readOam(uint16_t address) const {
    return oam[address - Specs::MemoryLayout::OAM::START];
}

void OamBus::writeOam(uint16_t address, uint8_t value) {
    oam[address - Specs::MemoryLayout::OAM::START] = value;
}

uint8_t OamBus::readFF(uint16_t address) const {
    return 0xFF;
}

void OamBus::writeNop(uint16_t address, uint8_t value) {
}
