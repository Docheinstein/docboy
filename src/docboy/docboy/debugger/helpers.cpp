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

uint8_t DebuggerHelpers::read_memory_raw(const GameBoy& gb, uint16_t address) {
    /* 0x0000 - 0x7FFF */
    if (address <= Specs::MemoryLayout::ROM1::END) {
        return gb.ext_bus.read_bus(address);
    }

    /* 0x8000 - 0x9FFF */
    if (address <= Specs::MemoryLayout::VRAM::END) {
        return gb.vram_bus.read_bus(address);
    }

    /* 0xA000 - 0xBFFF */
    if (address <= Specs::MemoryLayout::RAM::END) {
        return gb.ext_bus.read_bus(address);
    }

#ifdef ENABLE_CGB
    /* 0xC000 - 0xCFFF */
    if (address <= Specs::MemoryLayout::WRAM1::END) {
        return gb.wram_bus.read_bus(address);
    }
#else
    /* 0xC000 - 0xCFFF */
    if (address <= Specs::MemoryLayout::WRAM1::END) {
        return gb.ext_bus.read_bus(address);
    }
#endif

#ifdef ENABLE_CGB
    /* 0xD000 - 0xDFFF */
    if (address <= Specs::MemoryLayout::WRAM2::END) {
        return gb.wram_bus.read_bus(address);
    }
#else
    /* 0xD000 - 0xDFFF */
    if (address <= Specs::MemoryLayout::WRAM2::END) {
        return gb.ext_bus.read_bus(address);
    }
#endif

#ifdef ENABLE_CGB
    /* 0xE000 - 0xFDFF */
    if (address <= Specs::MemoryLayout::ECHO_RAM::END) {
        return gb.wram_bus.read_bus(address);
    }
#else
    /* 0xE000 - 0xFDFF */
    if (address <= Specs::MemoryLayout::ECHO_RAM::END) {
        return gb.ext_bus.read_bus(address);
    }
#endif

    /* 0xFE00 - 0xFE9F */
    if (address <= Specs::MemoryLayout::OAM::END) {
        return gb.oam[address - Specs::MemoryLayout::OAM::START];
    }

    /* 0xFEA0 - 0xFEFF */
    if (address <= Specs::MemoryLayout::NOT_USABLE::END) {
        return gb.oam_bus.read_bus(address);
    }

    /* 0xFF00 - 0xFFFF */
    if (address <= Specs::MemoryLayout::IO::END) {
        return gb.cpu_bus.read_bus(address);
    }

    /* 0xFF80 - 0xFFFE */
    if (address <= Specs::MemoryLayout::HRAM::END) {
        return gb.cpu_bus.read_bus(address);
    }

    /* 0xFFFF */
    return gb.cpu_bus.read_bus(address);
}
