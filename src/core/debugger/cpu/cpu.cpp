#include "cpu.h"

DebuggableCPU::DebuggableCPU(
        IBus &bus, IClockable &timers, IClockable &serial, bool bootRom) :
        CPU(bus, timers, serial, bootRom) {

}

ICPUDebug::State DebuggableCPU::getState() {
    return {
        .registers = {
            .AF = AF,
            .BC = BC,
            .HL = HL,
            .PC = PC,
            .SP = SP
        },
        .instruction = {
            .ISR = currentInstruction.ISR,
            .address = currentInstruction.address,
            .microop = currentInstruction.microop
        },
        .IME = IME,
        .halted = halted,
        .cycles = mCycles
    };
}
