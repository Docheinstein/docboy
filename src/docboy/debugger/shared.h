#ifndef SHARED_H
#define SHARED_H

#include <cstdint>
#include <variant>
#include <vector>

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

using Command = std::variant<DotCommand, StepCommand, MicroStepCommand, NextCommand, MicroNextCommand, FrameCommand,
                             FrameBackCommand, ScanlineCommand, ContinueCommand, AbortCommand>;

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
    AccessType accessType;
    uint8_t oldValue;
    uint8_t newValue;
};

using Disassemble = std::vector<uint8_t>;

struct ExecutionCompleted {};
struct ExecutionInterrupted {};
struct ExecutionBreakpointHit {
    BreakpointHit breakpointHit;
};
struct ExecutionWatchpointHit {
    WatchpointHit watchpointHit;
};

using ExecutionState =
    std::variant<ExecutionCompleted, ExecutionInterrupted, ExecutionBreakpointHit, ExecutionWatchpointHit>;

#endif // SHARED_H