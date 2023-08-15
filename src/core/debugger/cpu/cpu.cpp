#include "cpu.h"

DebuggableCPU::DebuggableCPU(IBus& bus, IClockable& timers, IClockable& serial, bool bootRom) :
    CPU(bus, timers, serial, bootRom) {
}

ICPUDebug::State DebuggableCPU::getState() {
    return {
        .registers =
            {
                .AF = AF,
                .BC = BC,
                .DE = DE,
                .HL = HL,
                .PC = PC,
                .SP = SP,
            },
        .instruction =
            {
                .ISR = currentInstruction.microopSelector == &CPU::ISR[0] ||
                       currentInstruction.microopSelector == &CPU::ISR[1] ||
                       currentInstruction.microopSelector == &CPU::ISR[2] ||
                       currentInstruction.microopSelector == &CPU::ISR[3] ||
                       currentInstruction.microopSelector == &CPU::ISR[4],
                .address = currentInstruction.address,
                .microop = currentInstruction.microop,
            },
        .IME = IME,
        .halted = halted,
        .cycles = mCycles,
    };
}
