#include "docboy/bus/oambus.h"

OamBus::OamBus(Oam& oam) :
    VideoBus {},
    oam {oam} {

    const NonTrivialReadFunctor read_ff = {[](void*, uint16_t) -> uint8_t {
                                               return 0xFF;
                                           },
                                           nullptr};
    const NonTrivialWriteFunctor write_nop = {[](void*, uint16_t, uint8_t) {
                                              },
                                              nullptr};
    const MemoryAccess open_bus_access {read_ff, write_nop};

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memory_accessors[i] = &oam[i - Specs::MemoryLayout::OAM::START];
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memory_accessors[i] = open_bus_access;
    }
}