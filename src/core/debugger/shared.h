#ifndef SHARED_H
#define SHARED_H

#include <variant>
#include <cstdint>
#include <vector>
#include "cpu/cpu.h"
#include "ppu/ppu.h"
#include "lcd/lcd.h"

namespace Debugger {
    struct CommandDot {
        uint64_t count;
    };
    struct CommandStep {
        uint64_t count;
    };
    struct CommandNext {
        uint64_t count;
    };
    struct CommandFrame {
        uint64_t count;
    };
    struct CommandContinue {};
    struct CommandAbort {};

    typedef std::variant<
        CommandDot,
        CommandStep,
        CommandNext,
        CommandFrame,
        CommandContinue,
        CommandAbort
    > Command;


    template<typename Op>
    struct LogicExpression {
        enum class Operator {
            Equal
        };
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
        enum class Type {
            Read,
            Write,
            ReadWrite,
            Change
        };

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
        enum class AccessType {
            Read,
            Write
        };
        Watchpoint watchpoint;
        uint16_t address;
        AccessType accessType;
        uint8_t oldValue;
        uint8_t newValue;
    };

    typedef std::vector<uint8_t> Disassemble;

    using CPUState = ICPUDebug::State;
    using PPUState = IPPUDebug::State;
    using LCDState = ILCDDebug::State;

    struct ExecutionState {
        enum class State {
            Completed,
            BreakpointHit,
            WatchpointHit,
            Interrupted
        } state;
        union {
            WatchpointHit watchpointHit;
            BreakpointHit breakpointHit;
        };
    };
}

#endif // SHARED_H