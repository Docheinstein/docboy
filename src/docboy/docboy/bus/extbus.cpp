#include "docboy/bus/extbus.h"
#include "docboy/cartridge/slot.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"

ExtBus::ExtBus(CartridgeSlot& cartridge_slot, Wram1& wram1, Wram2* wram2) :
    Bus {},
    cartridge_slot {cartridge_slot},
    wram1 {wram1},
    wram2 {wram2} {
    /* 0x0000 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::ROM0::START; i <= Specs::MemoryLayout::ROM1::END; i++) {
        memory_accessors[i] = {
            NonTrivialRead<&CartridgeSlot::read_rom> {&cartridge_slot},
            NonTrivialWrite<&CartridgeSlot::write_rom> {&cartridge_slot},
        };
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        memory_accessors[i] = {
            NonTrivialRead<&CartridgeSlot::read_ram> {&cartridge_slot},
            NonTrivialWrite<&CartridgeSlot::write_ram> {&cartridge_slot},

        };
    }

    /* 0xC000 - 0xCFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM1::START; i <= Specs::MemoryLayout::WRAM1::END; i++) {
        memory_accessors[i] = &wram1[i - Specs::MemoryLayout::WRAM1::START];
    }

    /* 0xD000 - 0xDFFF */
#ifdef ENABLE_CGB
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
        memory_accessors[i] = {
            NonTrivialRead<&ExtBus::read_wram2> {this},
            NonTrivialWrite<&ExtBus::write_wram2> {this},
        };
    }
#else
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
        memory_accessors[i] = &wram2[0][i - Specs::MemoryLayout::WRAM2::START];
    }
#endif

    /* 0xE000 - 0xFDFF */
    for (uint16_t i = Specs::MemoryLayout::ECHO_RAM::START; i <= Specs::MemoryLayout::ECHO_RAM::END; i++) {
        memory_accessors[i] = &wram1[i - Specs::MemoryLayout::ECHO_RAM::START];
    }

    reset();
}

void ExtBus::reset() {
    Bus::reset();
#ifdef ENABLE_CGB
    wram2_bank = 0;
#endif
}

#ifdef ENABLE_CGB
void ExtBus::set_wram2_bank(uint8_t bank) {
    ASSERT(bank < 8);
    wram2_bank = bank;
}

uint8_t ExtBus::read_wram2(uint16_t address) const {
    return wram2[wram2_bank][address - Specs::MemoryLayout::WRAM2::START];
}

void ExtBus::write_wram2(uint16_t address, uint8_t value) {
    wram2[wram2_bank][address - Specs::MemoryLayout::WRAM2::START] = value;
}
#endif