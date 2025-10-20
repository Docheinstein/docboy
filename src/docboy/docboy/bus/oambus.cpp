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
#ifdef ENABLE_CGB
    // On my CGB-E, it seems that the first 2 chunks of 16 bytes are fully readable/writable (from FEA0 to FEBF),
    // while the next 16 bytes chunks (FEC0 - FECF, FED0 - FEDF, FEE0 - FEEF, FEF0 - FEFF) are all mapped to the
    // the 16 byte chunk FEC0 - FECF.
    // TODO: is this pattern model specific?
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i < Specs::MemoryLayout::NOT_USABLE::START + 32; i++) {
        memory_accessors[i] = &not_usable[i - Specs::MemoryLayout::NOT_USABLE::START];
    }

    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START + 32; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memory_accessors[i] = &not_usable[32 + mod<16>(i)];
    }
#else
    for (uint16_t i = Specs::MemoryLayout::NOT_USABLE::START; i <= Specs::MemoryLayout::NOT_USABLE::END; i++) {
        memory_accessors[i] = {read_00, write_nop};
    }
#endif

    reset();
}

void OamBus::save_state(Parcel& parcel) const {
    VideoBus::save_state(parcel);

#ifndef ENABLE_CGB
    PARCEL_WRITE_BOOL(parcel, mcycle_write.happened);
    PARCEL_WRITE_UINT8(parcel, mcycle_write.previous_data);
#endif
}

void OamBus::load_state(Parcel& parcel) {
    VideoBus::load_state(parcel);

#ifndef ENABLE_CGB
    mcycle_write.happened = parcel.read_bool();
    mcycle_write.previous_data = parcel.read_uint8();
#endif
}

void OamBus::reset() {
    VideoBus::reset();

#ifndef ENABLE_CGB
    mcycle_write.happened = false;
    mcycle_write.previous_data = {};
#endif
}
