#include "docboy/debugger/helpers.h"

#include "docboy/cpu/cpu.h"
#include "docboy/gameboy/gameboy.h"

std::optional<uint8_t> DebuggerHelpers::get_isr_phase(const Cpu& cpu) {
    if (cpu.interrupt.state == Cpu::InterruptState::Pending && cpu.interrupt.remaining_ticks <= 1 &&
        cpu.instruction.microop.counter == 0 && cpu.ime == Cpu::ImeState::Enabled) {
        return 0;
    }

    if (cpu.instruction.microop.selector == &cpu.isr[0]) {
        return 0;
    }
    if (cpu.instruction.microop.selector == &cpu.isr[1]) {
        return 1;
    }
    if (cpu.instruction.microop.selector == &cpu.isr[2]) {
        return 2;
    }
    if (cpu.instruction.microop.selector == &cpu.isr[3]) {
        return 3;
    }
    if (cpu.instruction.microop.selector == &cpu.isr[4]) {
        return 4;
    }

    return std::nullopt;
}

bool DebuggerHelpers::is_in_isr(const Cpu& cpu) {
    return get_isr_phase(cpu) != std::nullopt;
}

uint8_t DebuggerHelpers::read_memory(const Mmu& mmu, uint16_t address) {
    return mmu.bus_accessors[Device::Cpu][address].read_bus(address);
}

uint8_t DebuggerHelpers::read_memory_raw(const GameBoy& gb, uint16_t bank, uint16_t address) {
    /* 0x0000 - 0x7FFF */
    if (address <= Specs::MemoryLayout::ROM1::END) {
        return gb.cartridge_slot.cartridge->read_rom_raw(bank, address);
    }

    /* 0x8000 - 0x9FFF */
    if (address <= Specs::MemoryLayout::VRAM::END) {
#ifdef ENABLE_CGB
        if (bank == 1) {
            return gb.vram1[address - Specs::MemoryLayout::VRAM::START];
        }
#endif
        ASSERT(bank == 0);
        return gb.vram0[address - Specs::MemoryLayout::VRAM::START];
    }

    /* 0xA000 - 0xBFFF */
    if (address <= Specs::MemoryLayout::RAM::END) {
        return gb.cartridge_slot.cartridge->read_ram_raw(bank, address);
    }

    /* 0xC000 - 0xCFFF */
    if (address <= Specs::MemoryLayout::WRAM1::END) {
        ASSERT(bank == 0);
        return gb.wram1[address - Specs::MemoryLayout::WRAM1::START];
    }

    /* 0xD000 - 0xDFFF */
    if (address <= Specs::MemoryLayout::WRAM2::END) {
#ifdef ENABLE_CGB
        ASSERT(bank < 7);
#else
        ASSERT(bank == 0);
#endif
        return gb.wram2[bank][address - Specs::MemoryLayout::WRAM2::START];
    }

    /* 0xE000 - 0xFDFF */
    if (address <= Specs::MemoryLayout::ECHO_RAM::END) {
        ASSERT(bank == 0);
        return gb.wram1[address - Specs::MemoryLayout::ECHO_RAM::START];
    }

    /* 0xFE00 - 0xFE9F */
    if (address <= Specs::MemoryLayout::OAM::END) {
        ASSERT(bank == 0);
        return gb.oam[address - Specs::MemoryLayout::OAM::START];
    }

    /* 0xFEA0 - 0xFEFF */
    if (address <= Specs::MemoryLayout::NOT_USABLE::END) {
        ASSERT(bank == 0);
        return gb.oam_bus.read_bus(address);
    }

    /* 0xFF00 - 0xFFFF */
    if (address <= Specs::MemoryLayout::IO::END) {
        ASSERT(bank == 0);
        return gb.cpu_bus.read_bus(address);
    }

    /* 0xFF80 - 0xFFFE */
    if (address <= Specs::MemoryLayout::HRAM::END) {
        ASSERT(bank == 0);
        return gb.hram[address - Specs::MemoryLayout::HRAM::START];
    }

    /* 0xFFFF */
    ASSERT(bank == 0);
    return gb.interrupts.IE;
}

uint16_t DebuggerHelpers::get_bank_for_address(const GameBoy& gb, uint16_t address) {
    /* 0x0000 - 0x7FFF */
    if (address <= Specs::MemoryLayout::ROM1::END) {
        return gb.cartridge_slot.cartridge->get_rom_bank(address);
    }

    /* 0x8000 - 0x9FFF */
    if (address <= Specs::MemoryLayout::VRAM::END) {
#ifdef ENABLE_CGB
        return gb.vram_bus.vram_bank;
#else
        return 0;
#endif
    }

    /* 0xA000 - 0xBFFF */
    if (address <= Specs::MemoryLayout::RAM::END) {
        return gb.cartridge_slot.cartridge->get_ram_bank(address);
    }

    /* 0xC000 - 0xCFFF */
    if (address <= Specs::MemoryLayout::WRAM1::END) {
        return 0;
    }

    /* 0xD000 - 0xDFFF */
    if (address <= Specs::MemoryLayout::WRAM2::END) {
#ifdef ENABLE_CGB
        return gb.wram_bus.wram2_bank;
#else
        return 0;
#endif
    }

    /* 0xE000 - 0xFDFF */
    if (address <= Specs::MemoryLayout::ECHO_RAM::END) {
        return 0;
    }

    /* 0xFE00 - 0xFE9F */
    if (address <= Specs::MemoryLayout::OAM::END) {
        return 0;
    }

    /* 0xFEA0 - 0xFEFF */
    if (address <= Specs::MemoryLayout::NOT_USABLE::END) {
        return 0;
    }

    /* 0xFF00 - 0xFFFF */
    if (address <= Specs::MemoryLayout::IO::END) {
        return 0;
    }

    /* 0xFF80 - 0xFFFE */
    if (address <= Specs::MemoryLayout::HRAM::END) {
        return 0;
    }

    /* 0xFFFF */
    return 0;
}
