#include "helpers.h"
#include "docboy/cpu/cpu.h"

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
    const Mmu::BusAccess& busAccess = mmu.busAccessors[address];
    return (*busAccess.read)(busAccess.bus, address);
}
