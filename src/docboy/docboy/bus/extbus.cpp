#include "docboy/bus/extbus.h"
#include "docboy/cartridge/slot.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"

ExtBus::ExtBus(CartridgeSlot& cartridge_slot, Wram1& wram1, Wram2& wram2) :
    Bus {},
    cartridge_slot {cartridge_slot},
    wram1 {wram1},
    wram2 {wram2} {

    /* 0x0000 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::ROM0::START; i <= Specs::MemoryLayout::ROM1::END; i++) {
        memory_accessors[i] = {
            NonTrivialRead<CartridgeSlot, &CartridgeSlot::read_rom> {&cartridge_slot},
            NonTrivialWrite<CartridgeSlot, &CartridgeSlot::write_rom> {&cartridge_slot},

        };
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        memory_accessors[i] = {
            NonTrivialRead<CartridgeSlot, &CartridgeSlot::read_ram> {&cartridge_slot},
            NonTrivialWrite<CartridgeSlot, &CartridgeSlot::write_ram> {&cartridge_slot},

        };
    }

    /* 0xC000 - 0xCFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM1::START; i <= Specs::MemoryLayout::WRAM1::END; i++) {
        memory_accessors[i] = &wram1[i - Specs::MemoryLayout::WRAM1::START];
    }

    /* 0xD000 - 0xDFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
        memory_accessors[i] = &wram2[i - Specs::MemoryLayout::WRAM2::START];
    }

    /* 0xE000 - 0xFDFF */
    for (uint16_t i = Specs::MemoryLayout::ECHO_RAM::START; i <= Specs::MemoryLayout::ECHO_RAM::END; i++) {
        memory_accessors[i] = &wram1[i - Specs::MemoryLayout::ECHO_RAM::START];
    }
}