#ifndef DEBUGGERHELPERS_H
#define DEBUGGERHELPERS_H

#include <cstdint>
#include <optional>

class GameBoy;
class Cpu;
class Mmu;

class DebuggerHelpers {
public:
    static std::optional<uint8_t> getIsrPhase(const Cpu& cpu);
    static bool isInIsr(const Cpu& cpu);

    static uint8_t readMemory(const Mmu& mmu, uint16_t address);
    static uint8_t readMemoryRaw(const GameBoy& gb, uint16_t address);
};

#endif // DEBUGGERHELPERS_H