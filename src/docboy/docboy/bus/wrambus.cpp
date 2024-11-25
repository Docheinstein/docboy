#include "docboy/bus/wrambus.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"

WramBus::WramBus(Wram1& wram1, Wram2* wram2) :
    Bus {},
    wram1 {wram1},
    wram2 {wram2} {
    /* 0xC000 - 0xCFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM1::START; i <= Specs::MemoryLayout::WRAM1::END; i++) {
        memory_accessors[i] = &wram1[i - Specs::MemoryLayout::WRAM1::START];
    }

    /* 0xD000 - 0xDFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
        memory_accessors[i] = {
            NonTrivialRead<&WramBus::read_wram2> {this},
            NonTrivialWrite<&WramBus::write_wram2> {this},
        };
    }

    /* 0xE000 - 0xFDFF */
    for (uint16_t i = Specs::MemoryLayout::ECHO_RAM::START; i <= Specs::MemoryLayout::ECHO_RAM::END; i++) {
        memory_accessors[i] = &wram1[i - Specs::MemoryLayout::ECHO_RAM::START];
    }

    reset();
}

void WramBus::set_wram2_bank(uint8_t bank) {
    ASSERT(bank < 8);
    wram2_bank = bank;
}

uint8_t WramBus::read_wram2(uint16_t address) const {
    ASSERT(address >= Specs::MemoryLayout::WRAM2::START && address <= Specs::MemoryLayout::WRAM2::END);
    return wram2[wram2_bank][address - Specs::MemoryLayout::WRAM2::START];
}

void WramBus::write_wram2(uint16_t address, uint8_t value) {
    ASSERT(address >= Specs::MemoryLayout::WRAM2::START && address <= Specs::MemoryLayout::WRAM2::END);
    wram2[wram2_bank][address - Specs::MemoryLayout::WRAM2::START] = value;
}

void WramBus::save_state(Parcel& parcel) const {
    Bus::save_state(parcel);
    parcel.write_uint8(wram2_bank);
}

void WramBus::load_state(Parcel& parcel) {
    Bus::load_state(parcel);
    wram2_bank = parcel.read_uint8();
}

void WramBus::reset() {
    Bus::reset();
    wram2_bank = 0;
}
