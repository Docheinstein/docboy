#include "cpu.h"


DebuggableCPU::DebuggableCPU(IBus &bus, SerialPort &serial, std::unique_ptr<IDebuggableBootROM> bootRom) :
    CPU(bus, serial, std::move(bootRom)) {

}

IDebuggableCPU::Registers DebuggableCPU::getRegisters() const {
    return {
        .AF = AF,
        .BC = BC,
        .HL = HL,
        .PC = PC,
        .SP = SP,
    };
}

IDebuggableCPU::Instruction DebuggableCPU::getCurrentInstruction() const {
    return {
        .ISR = currentInstruction.ISR,
        .address = currentInstruction.address,
        .microop = currentInstruction.microop
    };
}

bool DebuggableCPU::getIME() const {
    return IME;
}

bool DebuggableCPU::isHalted() const {
    return halted;
}

uint64_t DebuggableCPU::getCycles() const {
    return mCycles;
}

uint8_t DebuggableCPU::readMemoryThroughCPU(uint16_t addr) const {
    return CPU::readMemory(addr);
}

IDebuggableReadable *DebuggableCPU::getBootROM() const {
    return bootRom.get();
}

