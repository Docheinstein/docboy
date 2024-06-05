#include "docboy/bus/oambus.h"

OamBus::OamBus(Oam& oam) :
    VideoBus {},
    oam {oam} {

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memory_accessors[i] = {&OamBus::read_oam, &OamBus::write_oam};
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memory_accessors[i] = {&OamBus::read_ff, &OamBus::write_nop};
    }
}

uint8_t OamBus::read_oam(uint16_t address) const {
    return oam[address - Specs::MemoryLayout::OAM::START];
}

void OamBus::write_oam(uint16_t address, uint8_t value) {
    oam[address - Specs::MemoryLayout::OAM::START] = value;
}

uint8_t OamBus::read_ff(uint16_t address) const {
    return 0xFF;
}

void OamBus::write_nop(uint16_t address, uint8_t value) {
}
