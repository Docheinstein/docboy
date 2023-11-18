#ifndef DEBUGGERHELPERS_H
#define DEBUGGERHELPERS_H

#include <cstdint>
#include <optional>

class Cpu;

class DebuggerHelpers {
public:
    static std::optional<uint8_t> getIsrPhase(const Cpu& cpu);
    static bool isInIsr(const Cpu& cpu);
};

#endif // DEBUGGERHELPERS_H