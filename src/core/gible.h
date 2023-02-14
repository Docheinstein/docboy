#ifndef GIBLE_H
#define GIBLE_H

#include <string>
#include "memory.h"
#include "cartridge.h"
#include "cpu.h"
#include "bus.h"
#include "debugger/debuggerbackend.h"

class Gible : public DebuggerBackend {
public:
    Gible();
    ~Gible() override;

    bool loadROM(const std::string &rom);
    void start();

    void attachDebugger(DebuggerFrontend &frontend) override;
    void detachDebugger() override;
    void setBreakPoint(uint16_t addr) override;
    void unsetBreakPoint(uint16_t addr) override;
    bool hasBreakPoint(uint16_t addr) const override;
    void step() override;
    void next() override;
    void run() override;

    Instruction getCurrentInstruction() override;
    RegistersSnapshot getRegisters() override;
    std::vector<DisassembleEntry> disassemble(uint16_t from, uint16_t to) override;

private:
    Cartridge cartridge;
    Memory wram1;
    Memory wram2;
    Bus bus;
    CPU cpu;

    DebuggerFrontend *debugger;
    bool breakpoints[0xFFFF];

    void tick();
};
#endif // GIBLE_H