#ifndef SHARED_H
#define SHARED_H

#include "cpu/cpu.h"
#include "lcd/lcd.h"
#include "ppu/ppu.h"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace Debugger {
namespace Commands {
struct Dot {
    uint64_t count;
};
struct Step {
    uint64_t count;
};
struct MicroStep {
    uint64_t count;
};
struct Next {
    uint64_t count;
};
struct MicroNext {
    uint64_t count;
};
struct Frame {
    uint64_t count;
};
struct Continue {};
struct Abort {};
} // namespace Commands

typedef std::variant<Commands::Dot, Commands::Step, Commands::MicroStep, Commands::Next, Commands::MicroNext,
                     Commands::Frame, Commands::Continue, Commands::Abort>
    Command;

template <typename Op>
struct LogicExpression {
    enum class Operator { Equal };
    Operator operation;
    Op operand;
};

struct Breakpoint {
    uint32_t id;
    uint16_t address;
};

struct BreakpointHit {
    Breakpoint breakpoint;
};

struct Watchpoint {
    typedef LogicExpression<uint8_t> Condition;
    enum class Type { Read, Write, ReadWrite, Change };

    uint32_t id;
    Type type;
    struct {
        uint16_t from;
        uint16_t to;
    } address;
    struct {
        bool enabled;
        Condition condition;
    } condition; // avoid to use std::optional so that this remains POD
};

struct WatchpointHit {
    enum class AccessType { Read, Write };
    Watchpoint watchpoint;
    uint16_t address;
    AccessType accessType;
    uint8_t oldValue;
    uint8_t newValue;
};

struct MemoryReadError {
    //    ~MemoryReadError() = default;
    //    MemoryReadError(const MemoryReadError& other) = default;
    //    MemoryReadError& operator=(const MemoryReadError& other) = default;
    uint16_t address;
    std::string error;
};

struct MemoryWriteError {
    //    ~MemoryWriteError() = default;
    //    MemoryWriteError(const MemoryWriteError& other) = default;
    //    MemoryWriteError& operator=(const MemoryWriteError& other) = default;
    uint16_t address;
    std::string error;
};

typedef std::vector<uint8_t> Disassemble;

using CPUState = ICPUDebug::State;
using PPUState = IPPUDebug::State;
using LCDState = ILCDDebug::State;

namespace ExecutionStates {
struct Completed {};
struct Interrupted {};
struct BreakpointHit {
    Debugger::BreakpointHit breakpointHit;
};
struct WatchpointHit {
    Debugger::WatchpointHit watchpointHit;
};
struct MemoryReadError {
    Debugger::MemoryReadError readError;
};
struct MemoryWriteError {
    Debugger::MemoryWriteError writeError;
};
} // namespace ExecutionStates

typedef std::variant<ExecutionStates::Completed, ExecutionStates::Interrupted, ExecutionStates::BreakpointHit,
                     ExecutionStates::WatchpointHit, ExecutionStates::MemoryReadError,
                     ExecutionStates::MemoryWriteError>
    ExecutionState;

} // namespace Debugger

#endif // SHARED_H