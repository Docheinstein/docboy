#include "docboy/bus/vrambus.h"

#include "docboy/memory/vram0.h"
#ifdef ENABLE_CGB
#include "docboy/memory/vram1.h"
#endif

#include "docboy/bootrom/helpers.h"
#include "docboy/common/specs.h"

#ifdef ENABLE_CGB
VramBus::VramBus(Vram0& vram0, Vram1& vram1) :
#else
VramBus::VramBus(Vram0& vram0) :
#endif
    VideoBus {},
    vram0 {vram0}
#ifdef ENABLE_CGB
    ,
    vram1 {vram1}
#endif
{
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
        memory_accessors[i] = &vram0[i - Specs::MemoryLayout::VRAM::START];
    }
#endif

    reset();
}

void VramBus::save_state(Parcel& parcel) const {
    VideoBus::save_state(parcel);
    PARCEL_WRITE_UINT8(parcel, readers);
#ifdef ENABLE_CGB
    PARCEL_WRITE_BOOL(parcel, vram_bank);
#endif
}

void VramBus::load_state(Parcel& parcel) {
    VideoBus::load_state(parcel);
    readers = parcel.read_uint8();
#ifdef ENABLE_CGB
    vram_bank = parcel.read_bool();
#endif
}

void VramBus::reset() {
    VideoBus::reset();

    address = if_bootrom_else(0, 0x9904);
    data = if_bootrom_else(0xFF, 0x01);
    decay = if_bootrom_else(0, 6);

    readers = 0;
#ifdef ENABLE_CGB
    vram_bank = false;
#endif
}

#ifdef ENABLE_CGB
void VramBus::set_vram_bank(bool bank) {
    vram_bank = bank;
}
#endif

#ifdef ENABLE_CGB
uint8_t VramBus::read_vram(uint16_t address) const {
    if (vram_bank) {
        return vram1[address - Specs::MemoryLayout::VRAM::START];
    }
    return vram0[address - Specs::MemoryLayout::VRAM::START];
}

void VramBus::write_vram(uint16_t address, uint8_t value) {
    if (vram_bank) {
        vram1[address - Specs::MemoryLayout::VRAM::START] = value;
    } else {
        vram0[address - Specs::MemoryLayout::VRAM::START] = value;
    }
}
#endif
