#include "helpers.h"
#include "docboy/cpu/cpu.h"
#include "docboy/gameboy/gameboy.h"

std::optional<uint8_t> DebuggerHelpers::getIsrPhase(const Cpu& cpu) {
    if (cpu.interrupt.state == Cpu::InterruptState::Pending && cpu.interrupt.remainingTicks <= 1 &&
        cpu.instruction.microop.counter == 0 && cpu.IME == Cpu::ImeState::Enabled)
        return 0;

    if (cpu.instruction.microop.selector == &cpu.ISR[0])
        return 0;
    if (cpu.instruction.microop.selector == &cpu.ISR[1])
        return 1;
    if (cpu.instruction.microop.selector == &cpu.ISR[2])
        return 2;
    if (cpu.instruction.microop.selector == &cpu.ISR[3])
        return 3;
    if (cpu.instruction.microop.selector == &cpu.ISR[4])
        return 4;

    return std::nullopt;
}

bool DebuggerHelpers::isInIsr(const Cpu& cpu) {
    return getIsrPhase(cpu) != std::nullopt;
}

uint8_t DebuggerHelpers::readMemory(const Mmu& mmu, uint16_t address) {
    return mmu.busAccessors[Device::Cpu][address].readBus(address);
}

uint8_t DebuggerHelpers::readMemoryRaw(const GameBoy& gb, uint16_t address) {
    /* 0x0000 - 0x7FFF */
    if (address <= Specs::MemoryLayout::ROM1::END)
        return gb.extBus.readBus(address);

    /* 0x8000 - 0x9FFF */
    if (address <= Specs::MemoryLayout::VRAM::END)
        return gb.vram[address - Specs::MemoryLayout::VRAM::START];

    /* 0xA000 - 0xBFFF */
    if (address <= Specs::MemoryLayout::RAM::END)
        return gb.extBus.readBus(address);

    /* 0xC000 - 0xCFFF */
    if (address <= Specs::MemoryLayout::WRAM1::END)
        return gb.extBus.readBus(address);

    /* 0xD000 - 0xDFFF */
    if (address <= Specs::MemoryLayout::WRAM2::END)
        return gb.extBus.readBus(address);

    /* 0xE000 - 0xFDFF */
    if (address <= Specs::MemoryLayout::ECHO_RAM::END)
        return gb.extBus.readBus(address);

    /* 0xFE00 - 0xFE9F */
    if (address <= Specs::MemoryLayout::OAM::END)
        return gb.oam[address - Specs::MemoryLayout::OAM::START];

    /* 0xFEA0 - 0xFEFF */
    if (address <= Specs::MemoryLayout::NOT_USABLE::END)
        return gb.oamBus.readBus(address);

    /* 0xFF00 - 0xFFFF */
    if (address <= Specs::MemoryLayout::IO::END)
        return gb.cpuBus.readBus(address);

    /* 0xFF80 - 0xFFFE */
    if (address <= Specs::MemoryLayout::HRAM::END)
        return gb.cpuBus.readBus(address);

    /* 0xFFFF */
    return gb.cpuBus.readBus(address);
}
