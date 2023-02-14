#ifndef DEBUGGERBACKEND_H
#define DEBUGGERBACKEND_H

#include <cstdint>
#include <vector>
#include "instructions.h"

class DebuggerFrontend;

class DebuggerBackend {
public:
    struct RegistersSnapshot {
        uint16_t AF;
        uint16_t BC;
        uint16_t DE;
        uint16_t HL;
        uint16_t PC;
        uint16_t SP;
    };

    struct DisassembleEntry {
        InstructionInfo info;
        std::vector<uint8_t> instruction;
        uint16_t address;
    };

    struct Instruction {
        uint16_t address;
        uint8_t microop;
    };

    virtual ~DebuggerBackend();

    virtual void attachDebugger(DebuggerFrontend &frontend) = 0;
    virtual void detachDebugger() = 0;

    virtual void setBreakPoint(uint16_t addr) = 0;
    virtual void unsetBreakPoint(uint16_t addr) = 0;
    virtual void toggleBreakPoint(uint16_t addr);
    [[nodiscard]] virtual bool hasBreakPoint(uint16_t addr) const = 0;
    virtual void step() = 0;
    virtual void next() = 0;
    virtual void run() = 0;

    virtual Instruction getCurrentInstruction() = 0;
    virtual RegistersSnapshot getRegisters() = 0;
    virtual std::vector<DisassembleEntry> disassemble(uint16_t from, uint16_t to) = 0;
};
#endif // DEBUGGERBACKEND_H