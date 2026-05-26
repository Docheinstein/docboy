#ifndef DEBUGGERHELPERS_H
#define DEBUGGERHELPERS_H

#include <cstdint>
#include <optional>

class GameBoy;
class Cpu;
class Mmu;

class DebuggerHelpers {
public:
    static std::optional<uint8_t> get_isr_phase(const Cpu& cpu);
    static bool is_in_isr(const Cpu& cpu);

    static uint8_t read_memory(const Mmu& mmu, uint16_t address);
    static uint8_t read_memory_raw(const GameBoy& gb, uint16_t bank, uint16_t address);

    static uint16_t get_bank_for_address(const GameBoy& gb, uint16_t addr);

    static bool is_valid_banked_address(const GameBoy& gb, uint16_t bank, uint16_t address);
};

#endif // DEBUGGERHELPERS_H