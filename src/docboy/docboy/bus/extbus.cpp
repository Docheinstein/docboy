#include "extbus.h"
#include "docboy/cartridge/slot.h"
#include "docboy/memory/wram1.h"
#include "docboy/memory/wram2.h"

ExtBus::ExtBus(CartridgeSlot& cartridgeSlot, Wram1& wram1, Wram2& wram2) :
    Bus<ExtBus>(),
    cartridgeSlot(cartridgeSlot),
    wram1(wram1),
    wram2(wram2) {

    /* 0x0000 - 0x7FFF */
    for (uint16_t i = Specs::MemoryLayout::ROM0::START; i <= Specs::MemoryLayout::ROM1::END; i++) {
        memoryAccessors[i] = {&ExtBus::readCartridgeRom, &ExtBus::writeCartridgeRom};
    }

    /* 0xA000 - 0xBFFF */
    for (uint16_t i = Specs::MemoryLayout::RAM::START; i <= Specs::MemoryLayout::RAM::END; i++) {
        memoryAccessors[i] = {&ExtBus::readCartridgeRam, &ExtBus::writeCartridgeRam};
    }

    /* 0xC000 - 0xCFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM1::START; i <= Specs::MemoryLayout::WRAM1::END; i++) {
        memoryAccessors[i] = &wram1[i - Specs::MemoryLayout::WRAM1::START];
    }

    /* 0xD000 - 0xDFFF */
    for (uint16_t i = Specs::MemoryLayout::WRAM2::START; i <= Specs::MemoryLayout::WRAM2::END; i++) {
        memoryAccessors[i] = &wram2[i - Specs::MemoryLayout::WRAM2::START];
    }

    /* 0xE000 - 0xFDFF */
    for (uint16_t i = Specs::MemoryLayout::ECHO_RAM::START; i <= Specs::MemoryLayout::ECHO_RAM::END; i++) {
        memoryAccessors[i] = &wram1[i - Specs::MemoryLayout::ECHO_RAM::START];
    }
}

uint8_t ExtBus::readCartridgeRom(uint16_t address) const {
    return cartridgeSlot.readRom(address);
}

void ExtBus::writeCartridgeRom(uint16_t address, uint8_t value) {
    return cartridgeSlot.writeRom(address, value);
}

uint8_t ExtBus::readCartridgeRam(uint16_t address) const {
    return cartridgeSlot.readRam(address);
}

void ExtBus::writeCartridgeRam(uint16_t address, uint8_t value) {
    return cartridgeSlot.writeRam(address, value);
}