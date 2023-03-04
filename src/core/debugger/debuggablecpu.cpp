#include "debuggablecpu.h"

DebuggableCPU::DebuggableCPU(IBus &bus, std::unique_ptr<IBootROM> bootRom) :
    CPU(bus, std::move(bootRom)), observer() {

}

void DebuggableCPU::setObserver(DebuggableCPU::Observer *obs) {
    observer = obs;
}

void DebuggableCPU::unsetObserver() {
    observer = nullptr;
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

uint8_t DebuggableCPU::readMemory(uint16_t addr, bool notify) const {
    uint8_t value = CPU::readMemory(addr);
    if (notify && observer)
        observer->onMemoryRead(addr, value);
    return value;
}

void DebuggableCPU::writeMemory(uint16_t addr, uint8_t value, bool notify) {
    uint8_t oldValue = CPU::readMemory(addr);
    CPU::writeMemory(addr, value);
    if (notify && observer)
        observer->onMemoryWrite(addr, oldValue, value);
}

uint8_t DebuggableCPU::readMemoryRaw(uint16_t addr) const {
    return readMemory(addr, true); // from IDebuggableCPU
}


uint8_t DebuggableCPU::readMemory(uint16_t addr) const {
    return readMemory(addr, true); // from CPU
}

void DebuggableCPU::writeMemory(uint16_t addr, uint8_t value) {
    writeMemory(addr, value, true);
}
