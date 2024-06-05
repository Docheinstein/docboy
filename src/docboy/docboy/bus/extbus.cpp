#include "docboy/bus/extbus.h"
#include "docboy/cartridge/slot.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"

ExtBus::ExtBus(CartridgeSlot& cartridge_slot, Wram1& wram1, Wram2& wram2) :
    Bus<ExtBus> {},
    cartridge_slot {cartridge_slot},
    wram1 {wram1},
    wram2 {wram2} {

    /* 0x0000 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::ROM0::START; i <= Specs::MemoryLayout::ROM1::END; i++) {
        memory_accessors[i] = {&ExtBus::read_cartridge_rom, &ExtBus::write_cartridge_rom};
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        memory_accessors[i] = {&ExtBus::read_cartridge_ram, &ExtBus::write_cartridge_ram};
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

uint8_t ExtBus::read_cartridge_rom(uint16_t address) const {
    return cartridge_slot.read_rom(address);
}

void ExtBus::write_cartridge_rom(uint16_t address, uint8_t value) {
    return cartridge_slot.write_rom(address, value);
}

uint8_t ExtBus::read_cartridge_ram(uint16_t address) const {
    return cartridge_slot.read_ram(address);
}

void ExtBus::write_cartridge_ram(uint16_t address, uint8_t value) {
    return cartridge_slot.write_ram(address, value);
}