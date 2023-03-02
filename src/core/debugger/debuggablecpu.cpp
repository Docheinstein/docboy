#include "debuggablecpu.h"

DebuggableCpu::DebuggableCpu(IBus &bus) : Cpu(bus) {

}

IDebuggableCpu::Registers DebuggableCpu::getRegisters() const {
    return {
        .AF = AF,
        .BC = BC,
        .HL = HL,
        .PC = PC,
        .SP = SP,
    };
}

IDebuggableCpu::Instruction DebuggableCpu::getCurrentInstruction() const {
    return {
        .ISR = currentInstruction.ISR,
        .address = currentInstruction.address,
        .microop = currentInstruction.microop
    };
}

bool DebuggableCpu::getIME() const {
    return IME;
}

bool DebuggableCpu::isHalted() const {
    return halted;
}

uint64_t DebuggableCpu::getCycles() const {
    return mCycles;
}
