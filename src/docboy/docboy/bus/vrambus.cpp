#include "docboy/bus/vrambus.h"

#ifdef ENABLE_CGB
VramBus::VramBus(Vram& vram0, Vram& vram1) :
    VideoBus {},
    vram0 {vram0},
    vram1 {vram1} {
    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memory_accessors[i] = {
            NonTrivialRead<&VramBus::read_vram> {this},
            NonTrivialWrite<&VramBus::write_vram> {this},
        };
    }
}

#else
VramBus::VramBus(Vram& vram0) :
    VideoBus {},
    vram0 {vram0} {
    /* 0x8000 - 0x9FFF */
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memory_accessors[i] = &vram0[i - Specs::MemoryLayout::VRAM::START];
    }
}
#endif

void VramBus::save_state(Parcel& parcel) const {
    VideoBus::save_state(parcel);
#ifdef ENABLE_CGB
    parcel.write_bool(vram_bank);
#endif
}
void VramBus::load_state(Parcel& parcel) {
    VideoBus::load_state(parcel);
#ifdef ENABLE_CGB
    vram_bank = parcel.read_bool();
#endif
}

#ifdef ENABLE_CGB
void VramBus::switch_bank(bool bank) {
    vram_bank = bank;
}

uint8_t VramBus::read_vram(uint16_t address) const {
    return vram_bank ? vram1[address - Specs::MemoryLayout::VRAM::START]
                     : vram0[address - Specs::MemoryLayout::VRAM::START];
}

void VramBus::write_vram(uint16_t address, uint8_t value) {
    if (vram_bank) {
        vram1[address - Specs::MemoryLayout::VRAM::START] = value;
    } else {
        vram0[address - Specs::MemoryLayout::VRAM::START] = value;
    }
}
#endif
