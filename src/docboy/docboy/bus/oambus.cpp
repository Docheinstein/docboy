#include "docboy/bus/oambus.h"

#ifdef ENABLE_CGB
OamBus::OamBus(Oam& oam, NotUsable& not_usable) :
#else
OamBus::OamBus(Oam& oam) :
#endif
    VideoBus {},
    oam {oam}
#ifdef ENABLE_CGB
    ,
    not_usable {not_usable}
#endif
{

#ifndef ENABLE_CGB
    // clang-format off
    const NonTrivialReadFunctor read_00 {[](void*, uint16_t) -> uint8_t { return 0x00;}, nullptr};
    const NonTrivialWriteFunctor write_nop {[](void*, uint16_t, uint8_t) {}, nullptr};
    // clang-format on
#endif

    /* 0xFE00 - 0xFE9F */
    for (uint16_t i = Specs::MemoryLayout::OAM::START; i <= Specs::MemoryLayout::OAM::END; i++) {
        memory_accessors[i] = &oam[i - Specs::MemoryLayout::OAM::START];
    }

    /* 0xFEA0 - 0xFEFF */
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
#ifdef ENABLE_CGB
        memory_accessors[i] = &not_usable[i - Specs::MemoryLayout::NOT_USABLE::START];
#else
        memory_accessors[i] = {read_00, write_nop};
#endif
    }
}