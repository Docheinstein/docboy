#include "docboy/bus/vrambus.h"

VramBus::VramBus(Vram* vram) :
    VideoBus {},
    vram {vram} {
    /* 0x8000 - 0x9FFF */
#ifdef ENABLE_CGB
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memory_accessors[i] = {
            NonTrivialRead<&VramBus::read_vram> {this},
            NonTrivialWrite<&VramBus::write_vram> {this},
        };
    }
#else
    for (uint16_t i = Specs::MemoryLayout::VRAM::START; i <= Specs::MemoryLayout::VRAM::END; i++) {
        memory_accessors[i] = &vram[0][i - Specs::MemoryLayout::VRAM::START];
    }
#endif
}

#ifdef ENABLE_CGB
void VramBus::set_vram_bank(bool bank) {
    vram_bank = bank;
}
#endif

void VramBus::save_state(Parcel& parcel) const {
    VideoBus::save_state(parcel);
    parcel.write_uint8(readers);
#ifdef ENABLE_CGB
    parcel.write_bool(vram_bank);
#endif
}

void VramBus::load_state(Parcel& parcel) {
    VideoBus::load_state(parcel);
    readers = parcel.read_uint8();
#ifdef ENABLE_CGB
    vram_bank = parcel.read_bool();
#endif
}

#ifdef ENABLE_CGB
uint8_t VramBus::read_vram(uint16_t address) const {
    return vram[vram_bank][address - Specs::MemoryLayout::VRAM::START];
}

void VramBus::write_vram(uint16_t address, uint8_t value) {
    vram[vram_bank][address - Specs::MemoryLayout::VRAM::START] = value;
}
#endif
