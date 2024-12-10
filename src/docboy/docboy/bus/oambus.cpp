#include "docboy/bus/oambus.h"

OamBus::OamBus(Oam& oam) :
    VideoBus {},
    oam {oam} {

    // clang-format off
    const NonTrivialReadFunctor read_00 {[](void*, uint16_t) -> uint8_t { return 0x00;}, nullptr};
    const NonTrivialWriteFunctor write_nop {[](void*, uint16_t, uint8_t) {}, nullptr};
    // clang-format on

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memory_accessors[i] = &oam[i - Specs::MemoryLayout::OAM::START];
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memory_accessors[i] = {read_00, write_nop};
    }
}