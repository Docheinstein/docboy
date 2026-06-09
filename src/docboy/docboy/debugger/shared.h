#ifndef SHARED_H
#define SHARED_H

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct Bank {
    bool operator==(const Bank& other) const {
        return bank == other.bank
#ifdef ENABLE_BOOTROM
               && boot == other.boot
#endif
            ;
    }

#ifdef ENABLE_BOOTROM
    bool boot {};
#endif
    uint16_t bank {};
};

#ifdef ENABLE_BOOTROM
struct BootBank : Bank {
    BootBank() :
        Bank {true, 0} {
    }
};
#endif

#ifdef ENABLE_BOOTROM
struct NumberedBank : Bank {
    explicit NumberedBank(uint16_t bank) :
        Bank {false, bank} {
    }
};
#else
using NumberedBank = Bank;
#endif

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

template <typename Operand>
struct LogicExpression {
    enum class Operator { Equal };
    Operator operation;
    Operand operand;
};

struct Breakpoint {
    uint32_t id {};
    Bank bank {};
    uint16_t address {};
};

struct BreakpointHit {
    Breakpoint breakpoint;
};

struct Watchpoint {
    using Condition = LogicExpression<uint8_t>;
    enum class Type { Read, Write, ReadWrite, Change };

    uint32_t id {};
    Type type {};
    Bank bank {};
    struct {
        uint16_t from {};
        uint16_t to {};
    } address;
    bool raw {};
    std::optional<Condition> condition {};
};

struct WatchpointHit {
    enum class AccessType { Read, Write };
    Watchpoint watchpoint {};
    uint16_t address {};
    AccessType access_type {};
    uint8_t old_value {};
    uint8_t new_value {};
};

using DisassembledInstruction = std::vector<uint8_t>;

struct DisassembledInstructionRef {
    DisassembledInstructionRef(const Bank bank, const uint16_t address, const DisassembledInstruction& instruction) :
        bank {bank},
        address {address},
        instruction {instruction} {
    }
    DisassembledInstructionRef(const Bank bank, const uint16_t address, DisassembledInstruction&& instruction) :
        bank {bank},
        address {address},
        instruction {std::move(instruction)} {
    }

    Bank bank {};
    uint16_t address {};
    DisassembledInstruction instruction {};
};

struct CartridgeInfo {
    std::string title {};
    uint8_t mbc {};
    uint8_t rom {};
    uint8_t ram {};
    bool multicart {};
    uint8_t cksum {};
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

// Entry of a .sym file
struct DebugSymbol {
    Bank bank {};
    uint16_t address {};
    std::string name {};
};
#endif // SHARED_H