#ifndef BACKEND_H
#define BACKEND_H

#include "shared.h"
#include <cstdint>
#include <deque>
#include <optional>

class DebuggerFrontend;
class Core;
class GameBoy;

struct TickCommandState {
    uint64_t target {};
};
struct DotCommandState {
    uint64_t target {};
};
struct StepCommandState {
    uint64_t counter {};
};
struct MicroStepCommandState {
    uint64_t counter {};
};
struct NextCommandState {
    uint64_t counter {};
    uint16_t stackLevel {};
};
struct MicroNextCommandState {
    uint64_t counter {};
    uint16_t stackLevel {};
};
struct FrameCommandState {
    uint64_t counter {};
};
struct FrameBackCommandState {
    uint64_t counter {};
};
struct ScanlineCommandState {
    uint64_t counter {};
};
struct ContinueCommandState {};
struct AbortCommandState {};

using CommandState = std::variant<TickCommandState, DotCommandState, StepCommandState, MicroStepCommandState,
                                  NextCommandState, MicroNextCommandState, FrameCommandState, FrameBackCommandState,
                                  ScanlineCommandState, ContinueCommandState, AbortCommandState>;

class DebuggerBackend {
public:
    explicit DebuggerBackend(Core& core);

    void attachFrontend(DebuggerFrontend& frontend);

    void onTick(uint64_t tick);
    void onMemoryRead(uint16_t address, uint8_t value);
    void onMemoryWrite(uint16_t address, uint8_t oldValue, uint8_t newValue);

    [[nodiscard]] bool isAskingToShutdown() const;

    uint32_t addBreakpoint(uint16_t addr);
    [[nodiscard]] std::optional<Breakpoint> getBreakpoint(uint16_t addr) const;
    [[nodiscard]] const std::vector<Breakpoint>& getBreakpoints() const;

    uint32_t addWatchpoint(Watchpoint::Type, uint16_t from, uint16_t to, std::optional<Watchpoint::Condition>);
    [[nodiscard]] std::optional<Watchpoint> getWatchpoint(uint16_t addr) const;
    [[nodiscard]] const std::vector<Watchpoint>& getWatchpoints() const;

    void removePoint(uint32_t id);
    void clearPoints();

    void disassemble(uint16_t addr, size_t n);
    void disassembleRange(uint16_t from, uint16_t to);
    [[nodiscard]] std::optional<DisassembledInstruction> getDisassembledInstruction(uint16_t addr) const;
    [[nodiscard]] std::vector<std::pair<uint16_t, DisassembledInstruction>> getDisassembledInstructions() const;

    [[nodiscard]] uint8_t readMemory(uint16_t addr);

    void proceed();
    void interrupt();

    [[nodiscard]] const Core& getCore() const;

private:
    void pullCommand(const ExecutionState& state);

    [[nodiscard]] std::optional<DisassembledInstruction> doDisassemble(uint16_t addr);

    template <typename CommandType>
    void initializeCommandState();

    template <typename CommandType>
    void initializeCommandState(const CommandType& cmd);

    template <typename CommandType, typename CommandStateType>
    void handleCommand();

    template <typename CommandType, typename CommandStateType>
    void handleCommand(const CommandType& cmd, CommandStateType& state);

    Core& core;
    DebuggerFrontend* frontend {};

    std::optional<Command> command;
    std::optional<CommandState> commandState;

    bool run {true};
    bool interrupted {};

    std::vector<Breakpoint> breakpoints;
    std::vector<Watchpoint> watchpoints;
    std::optional<DisassembledInstruction> disassembledInstructions[0x10000];

    std::optional<WatchpointHit> watchpointHit;

    uint32_t nextPointId {};

    bool allowMemoryCallbacks {true};

    std::deque<std::vector<uint8_t>> history;
};

#endif // BACKEND_H