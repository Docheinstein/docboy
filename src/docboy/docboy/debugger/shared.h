#ifndef SHARED_H
#define SHARED_H

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

struct TickCommand {
    uint64_t count;
};
struct DotCommand {
    uint64_t count;
};
struct StepCommand {
    uint64_t count;
};
struct MicroStepCommand {
    uint64_t count;
};
struct NextCommand {
    uint64_t count;
};
struct MicroNextCommand {
    uint64_t count;
};
struct FrameCommand {
    uint64_t count;
};
struct FrameBackCommand {
    uint64_t count;
};
struct ScanlineCommand {
    uint64_t count;
};
struct ContinueCommand {};
struct AbortCommand {};

using Command = std::variant<TickCommand, DotCommand, StepCommand, MicroStepCommand, NextCommand, MicroNextCommand,
                             FrameCommand, FrameBackCommand, ScanlineCommand, ContinueCommand, AbortCommand>;

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
    using Condition = LogicExpression<uint8_t>;
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
    AccessType access_type;
    uint8_t old_value;
    uint8_t new_value;
};

using DisassembledInstruction = std::vector<uint8_t>;

struct DisassembledInstructionReference {
    DisassembledInstructionReference(uint16_t address, const DisassembledInstruction& instruction) :
        address {address},
        instruction {instruction} {
    }

    DisassembledInstructionReference(uint16_t address, DisassembledInstruction&& instruction) :
        address {address},
        instruction {std::move(instruction)} {
    }

    uint16_t address;
    DisassembledInstruction instruction;
};

struct CartridgeInfo {
    std::string title {};
    uint8_t mbc {};
    uint8_t rom {};
    uint8_t ram {};
    bool multicart {};
};

struct ExecutionCompleted {};
struct ExecutionInterrupted {};
struct ExecutionBreakpointHit {
    BreakpointHit breakpoint_hit;
};
struct ExecutionWatchpointHit {
    WatchpointHit watchpoint_hit;
};

using ExecutionState =
    std::variant<ExecutionCompleted, ExecutionInterrupted, ExecutionBreakpointHit, ExecutionWatchpointHit>;

#endif // SHARED_H