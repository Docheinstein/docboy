#ifndef DEBUGGERBACKEND_H
#define DEBUGGERBACKEND_H

#include <cstdint>
#include <vector>
#include <optional>
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
    struct FlagsSnapshot {
        bool Z;
        bool N;
        bool H;
        bool C;
    };

    struct DisassembleEntry {
        std::vector<uint8_t> instruction;
        uint16_t address;
    };

    template<typename Op>
    struct Condition {
        enum class Operator {
            Equal
        };
        Operator operation;
        Op operand;
    };

    struct Instruction {
        uint16_t address;
        uint8_t microop;
    };

    struct Breakpoint {
        uint32_t id;
        uint16_t address;
    };

    struct Watchpoint {
        typedef DebuggerBackend::Condition<uint8_t> Condition;
        enum class Type {
            Read,
            Write,
            ReadWrite,
            Change
        };

        uint32_t id;
        Type type;
        uint16_t from;
        uint16_t to;
        std::optional<Condition> condition;
    };

    struct Cyclepoint {
        uint32_t id;
        uint64_t n;
    };

    struct BreakpointTrigger {
        Breakpoint breakpoint;
    };

    struct CyclepointTrigger {
        Cyclepoint cyclepoint;
    };

    struct WatchpointTrigger {
        enum class Type {
            Read,
            Write
        };
        Watchpoint watchpoint;
        uint16_t address;
        Type type;
        uint8_t oldValue;
        uint8_t newValue;
    };

    struct ExecResult {
        enum class EndReason {
            Completed,
            Breakpoint,
            Watchpoint,
            Cyclepoint,
            Interrupted
        };
        EndReason reason;
        union {
            WatchpointTrigger watchpointTrigger;
            BreakpointTrigger breakpointTrigger;
            CyclepointTrigger cyclepointTrigger;
        };
    };

//    typedef std::variant<TriggeredBreakpoint, TriggeredWatchpoint> PauseReason;

    virtual ~DebuggerBackend();

    virtual void attachDebugger(DebuggerFrontend &frontend) = 0;
    virtual void detachDebugger() = 0;

    virtual uint32_t addBreakpoint(uint16_t addr) = 0;
    [[nodiscard]] virtual std::optional<Breakpoint> getBreakpoint(uint16_t addr) const = 0;
    [[nodiscard]] virtual const std::vector<Breakpoint> & getBreakpoints() const = 0;

    virtual uint32_t addWatchpoint(Watchpoint::Type, uint16_t from, uint16_t to, std::optional<Watchpoint::Condition>) = 0;
    [[nodiscard]] virtual std::optional<Watchpoint> getWatchpoint(uint16_t addr) const = 0;
    [[nodiscard]] virtual const std::vector<Watchpoint> & getWatchpoints() const = 0;

    virtual uint32_t addCyclepoint(uint64_t n) = 0;
    [[nodiscard]] virtual std::optional<Cyclepoint> getCyclepoint(uint64_t n) const = 0;
    [[nodiscard]] virtual const std::vector<Cyclepoint> & getCyclepoints() const = 0;

    virtual void removePoint(uint32_t id) = 0;
    virtual void clearPoints() = 0;


//    virtual void unsetBreakpoint(uint16_t addr) = 0;
//    [[nodiscard]] virtual std::optional<Breakpoint> getBreakpoint(uint16_t addr) const = 0;
//    virtual void removeWatchpoint(uint32_t id) = 0;
//    virtual void clearWatchpoints() = 0;



//    virtual void unsetWatchpoint(uint16_t addr) = 0;
//    [[nodiscard]] virtual bool hasWatchpoint(uint16_t addr) const = 0;
//    [[nodiscard]] virtual std::optional<Watchpoint> getWatchpoint(uint16_t addr) const = 0;

    [[nodiscard]] virtual std::optional<DisassembleEntry> getCode(uint16_t addr) const = 0;
    [[nodiscard]] virtual const std::vector<DisassembleEntry> & getCode() const = 0;

    virtual void disassemble(uint16_t addr, size_t n) = 0;
    virtual void disassembleRange(uint16_t from, uint16_t to) = 0;

//    virtual PauseReason run() = 0;

    virtual Instruction getCurrentInstruction() = 0;
    virtual RegistersSnapshot getRegisters() = 0;
    virtual FlagsSnapshot getFlags() = 0;
    virtual uint64_t getCurrentCycle() = 0;
    virtual uint64_t getCurrentMcycle() = 0;

    virtual uint8_t readMemory(uint16_t addr) = 0;


    virtual ExecResult step() = 0;
    virtual ExecResult next() = 0;
    virtual ExecResult continue_() = 0;

    virtual void reset() = 0;
};
#endif // DEBUGGERBACKEND_H