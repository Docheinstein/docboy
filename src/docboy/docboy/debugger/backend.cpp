#include "backend.h"
#include "docboy/core/core.h"
#include "frontend.h"
#include "helpers.h"
#include "mnemonics.h"
#include "utils/std.hpp"
#include <algorithm>

namespace {
constexpr uint32_t MAX_HISTORY_SIZE = 600; // ~10 sec
}

DebuggerBackend::DebuggerBackend(Core& core) :
    core(core) {
}

void DebuggerBackend::attachFrontend(DebuggerFrontend& frontend_) {
    frontend = &frontend_;
}

void DebuggerBackend::onTick(uint64_t tick) {
    if (!frontend)
        return;

    Cpu& cpu = core.gb.cpu;

    // Notify the frontend for extra actions to take (e.g. handle CTRL+C, tracing)
    frontend->onTick(tick);

    if (interrupted) {
        // User interruption (CTRL+C): pull command again
        interrupted = false;
        pullCommand(ExecutionInterrupted());
        return;
    }

    // Cache the state at each new frame to allow rewinding with FrameBack command
    bool isNewFrame = core.gb.ppu.tickSelector == &Ppu::vBlank && core.gb.video.LY == 144 && core.gb.ppu.dots == 0;
    if (isNewFrame) {
        // Eventually make space for the new state
        if (history.size() == MAX_HISTORY_SIZE)
            history.pop_front();

        // Save state and push it to the history
        std::vector<uint8_t> state;
        state.resize(core.getStateSize());
        core.saveState(state.data());

        history.push_back(std::move(state));
        check(history.size() <= MAX_HISTORY_SIZE);
    }

    bool isMCycle = (tick % 4) == 0;

    // Check breakpoints
    if (isMCycle) {
        if (!DebuggerHelpers::isInIsr(cpu) && cpu.instruction.microop.counter == 0) {
            // Disassemble the current instruction
            disassemble(cpu.instruction.address, 1);
            auto b = getBreakpoint(cpu.instruction.address);
            if (b) {
                // A breakpoint has been triggered: pull command again
                pullCommand(ExecutionBreakpointHit {*b});
                return;
            }
        }
    }

    // Check watchpoints
    if (watchpointHit) {
        // A watchpoint has been triggered: pull command again
        ExecutionWatchpointHit state {*watchpointHit};
        watchpointHit = std::nullopt;
        pullCommand(state);
        return;
    }

    if (!command) {
        // No command yet: pull command
        pullCommand(ExecutionCompleted());
        return;
    }

    // Handle current command

    const Command& cmd = *command;

    // T-Cycle commands (PPU)
    if (std::holds_alternative<TickCommand>(cmd)) {
        handleCommand<TickCommand, TickCommandState>();
    } else if (std::holds_alternative<DotCommand>(cmd)) {
        handleCommand<DotCommand, DotCommandState>();
    } else if (std::holds_alternative<FrameCommand>(cmd)) {
        handleCommand<FrameCommand, FrameCommandState>();
    } else if (std::holds_alternative<FrameBackCommand>(cmd)) {
        handleCommand<FrameBackCommand, FrameBackCommandState>();
    } else if (std::holds_alternative<ScanlineCommand>(cmd)) {
        handleCommand<ScanlineCommand, ScanlineCommandState>();
    } else if (isMCycle) {
        // M-Cycle commands (CPU)
        if (std::holds_alternative<DotCommand>(cmd)) {
            handleCommand<DotCommand, DotCommandState>();
        } else if (std::holds_alternative<StepCommand>(cmd)) {
            handleCommand<StepCommand, StepCommandState>();
        } else if (std::holds_alternative<MicroStepCommand>(cmd)) {
            handleCommand<MicroStepCommand, MicroStepCommandState>();
        } else if (std::holds_alternative<NextCommand>(cmd)) {
            handleCommand<NextCommand, NextCommandState>();
        } else if (std::holds_alternative<MicroNextCommand>(cmd)) {
            handleCommand<MicroNextCommand, MicroNextCommandState>();
        } else if (std::holds_alternative<FrameCommand>(cmd)) {
            handleCommand<FrameCommand, FrameCommandState>();
        } else if (std::holds_alternative<FrameBackCommand>(cmd)) {
            handleCommand<FrameBackCommand, FrameBackCommandState>();
        } else if (std::holds_alternative<ContinueCommand>(cmd)) {
            handleCommand<ContinueCommand, ContinueCommandState>();
        } else if (std::holds_alternative<AbortCommand>(cmd)) {
            handleCommand<AbortCommand, AbortCommandState>();
        }
    }
}

void DebuggerBackend::onMemoryRead(uint16_t address, uint8_t value) {
    if (!allowMemoryCallbacks)
        return;

    std::optional<Watchpoint> w = getWatchpoint(address);
    if (w) {
        if ((w->type == Watchpoint::Type::Read || w->type == Watchpoint::Type::ReadWrite) &&
            (!w->condition.enabled || (w->condition.condition.operation == Watchpoint::Condition::Operator::Equal &&
                                       value == w->condition.condition.operand))) {
            watchpointHit = WatchpointHit();
            watchpointHit->watchpoint = *w;
            watchpointHit->address = address;
            watchpointHit->accessType = WatchpointHit::AccessType::Read;
            watchpointHit->oldValue = value;
            watchpointHit->newValue = value;
        }
    }
}

void DebuggerBackend::onMemoryWrite(uint16_t address, uint8_t oldValue, uint8_t newValue) {
    if (!allowMemoryCallbacks)
        return;

    std::optional<Watchpoint> w = getWatchpoint(address);
    if (w) {
        if (((w->type == Watchpoint::Type::ReadWrite || w->type == Watchpoint::Type::Write) ||
             (w->type == Watchpoint::Type::Change && oldValue != newValue)) &&
            (!w->condition.enabled || (w->condition.condition.operation == Watchpoint::Condition::Operator::Equal &&
                                       newValue == w->condition.condition.operand))) {
            watchpointHit = WatchpointHit();
            watchpointHit->watchpoint = *w;
            watchpointHit->address = address;
            watchpointHit->accessType = WatchpointHit::AccessType::Write;
            watchpointHit->oldValue = oldValue;
            watchpointHit->newValue = newValue;
        }
    }
}

bool DebuggerBackend::isAskingToShutdown() const {
    return !run;
}

uint32_t DebuggerBackend::addBreakpoint(uint16_t addr) {
    const uint32_t id = nextPointId++;
    const Breakpoint b {id, addr};
    breakpoints.push_back(b);
    return id;
}

std::optional<Breakpoint> DebuggerBackend::getBreakpoint(uint16_t addr) const {
    auto b = std::find_if(breakpoints.begin(), breakpoints.end(), [addr](const Breakpoint& b) {
        return b.address == addr;
    });
    if (b != breakpoints.end())
        return *b;
    return std::nullopt;
}

const std::vector<Breakpoint>& DebuggerBackend::getBreakpoints() const {
    return breakpoints;
}

uint32_t DebuggerBackend::addWatchpoint(Watchpoint::Type type, uint16_t from, uint16_t to,
                                        std::optional<Watchpoint::Condition> cond) {
    const uint32_t id = nextPointId++;
    const Watchpoint w {id,
                        type,
                        {.from = from, .to = to},
                        {.enabled = static_cast<bool>(cond), .condition = cond ? *cond : Watchpoint::Condition()}};
    watchpoints.push_back(w);
    return id;
}

std::optional<Watchpoint> DebuggerBackend::getWatchpoint(uint16_t addr) const {
    auto w = std::find_if(watchpoints.begin(), watchpoints.end(), [addr](const Watchpoint& wp) {
        return wp.address.from <= addr && addr <= wp.address.to;
    });
    if (w != watchpoints.end())
        return *w;
    return std::nullopt;
}

const std::vector<Watchpoint>& DebuggerBackend::getWatchpoints() const {
    return watchpoints;
}

void DebuggerBackend::removePoint(uint32_t id) {
    erase_if(breakpoints, [id](const Breakpoint& b) {
        return b.id == id;
    });
    erase_if(watchpoints, [id](const Watchpoint& w) {
        return w.id == id;
    });
}

void DebuggerBackend::clearPoints() {
    breakpoints.clear();
    watchpoints.clear();
}

std::optional<DisassembledInstructionReference> DebuggerBackend::disassemble(uint16_t addr, bool cache) {
    auto instrs = disassembleMultiple(addr, 1, cache);
    if (instrs.empty())
        return {};
    return instrs[0];
}

std::vector<DisassembledInstructionReference> DebuggerBackend::disassembleMultiple(uint16_t addr, uint16_t n,
                                                                                   bool cache) {
    std::vector<DisassembledInstructionReference> refs;

    uint32_t addressCursor = addr;
    for (uint32_t i = 0; i < n && addressCursor <= 0xFFFF; i++) {
        auto instruction = doDisassemble(addressCursor);
        if (!instruction)
            break;
        if (cache)
            disassembledInstructions[addressCursor] = instruction;
        refs.emplace_back(addressCursor, *instruction);
        addressCursor += instruction->size();
    }

    return refs;
}

std::vector<DisassembledInstructionReference> DebuggerBackend::disassembleRange(uint16_t from, uint16_t to,
                                                                                bool cache) {
    std::vector<DisassembledInstructionReference> refs;

    for (uint32_t addressCursor = from; addressCursor <= to && addressCursor <= 0xFFFF;) {
        auto instruction = doDisassemble(addressCursor);
        if (!instruction)
            break;
        if (cache)
            disassembledInstructions[addressCursor] = instruction;
        refs.emplace_back(addressCursor, *instruction);
        addressCursor += instruction->size();
    }

    return refs;
}

std::optional<DisassembledInstruction> DebuggerBackend::getDisassembledInstruction(uint16_t addr) const {
    return disassembledInstructions[addr];
}

std::vector<DisassembledInstructionReference> DebuggerBackend::getDisassembledInstructions() const {
    std::vector<DisassembledInstructionReference> refs;
    for (uint32_t addr = 0; addr < array_size(disassembledInstructions); addr++) {
        if (disassembledInstructions[addr]) {
            refs.emplace_back(addr, *disassembledInstructions[addr]);
        }
    }
    return refs;
}

uint8_t DebuggerBackend::readMemory(uint16_t addr) {
    allowMemoryCallbacks = false;
    uint8_t value = DebuggerHelpers::readMemory(core.gb.mmu, addr);
    allowMemoryCallbacks = true;
    return value;
}

uint8_t DebuggerBackend::readMemoryRaw(uint16_t addr) {
    allowMemoryCallbacks = false;
    uint8_t value = DebuggerHelpers::readMemoryRaw(core.gb, addr);
    allowMemoryCallbacks = true;
    return value;
}

std::optional<DisassembledInstruction> DebuggerBackend::doDisassemble(uint16_t addr) {
    uint32_t addressCursor = addr;
    uint8_t opcode = readMemory(addressCursor++);
    DisassembledInstruction instruction {opcode};

    // this works for CB and non CB because all CB instructions have length 2
    uint8_t length = instruction_length(opcode);

    while (addressCursor < addr + length && addressCursor <= 0xFFFF)
        instruction.push_back(readMemory(addressCursor++));

    return instruction;
}

void DebuggerBackend::pullCommand(const ExecutionState& state) {
    command = frontend->pullCommand(state);

    const Command& cmd = *command;

    if (std::holds_alternative<TickCommand>(cmd)) {
        initializeCommandState<TickCommand>();
    } else if (std::holds_alternative<DotCommand>(cmd)) {
        initializeCommandState<DotCommand>();
    } else if (std::holds_alternative<StepCommand>(cmd)) {
        initializeCommandState<StepCommand>();
    } else if (std::holds_alternative<MicroStepCommand>(cmd)) {
        initializeCommandState<MicroStepCommand>();
    } else if (std::holds_alternative<NextCommand>(cmd)) {
        initializeCommandState<NextCommand>();
    } else if (std::holds_alternative<MicroNextCommand>(cmd)) {
        initializeCommandState<MicroNextCommand>();
    } else if (std::holds_alternative<FrameCommand>(cmd)) {
        initializeCommandState<FrameCommand>();
    } else if (std::holds_alternative<FrameBackCommand>(cmd)) {
        initializeCommandState<FrameBackCommand>();
    } else if (std::holds_alternative<ScanlineCommand>(cmd)) {
        initializeCommandState<ScanlineCommand>();
    } else if (std::holds_alternative<ContinueCommand>(cmd)) {
        initializeCommandState<ContinueCommand>();
    } else if (std::holds_alternative<AbortCommand>(cmd)) {
        initializeCommandState<AbortCommand>();
    }
}

void DebuggerBackend::interrupt() {
    interrupted = true;
}

const Core& DebuggerBackend::getCore() const {
    return core;
}

void DebuggerBackend::proceed() {
    command = ContinueCommand();
    initializeCommandState<ContinueCommand>();
}

// ===================== COMMANDS STATE INITIALIZATION =========================

template <typename CommandType>
void DebuggerBackend::initializeCommandState() {
    initializeCommandState(std::get<CommandType>(*command));
}

// Tick
template <>
void DebuggerBackend::initializeCommandState<TickCommand>(const TickCommand& cmd) {
    commandState = TickCommandState {core.ticks + cmd.count};
}

// Dot
template <>
void DebuggerBackend::initializeCommandState<DotCommand>(const DotCommand& cmd) {
    commandState = DotCommandState {core.gb.ppu.cycles + cmd.count};
}

// Step
template <>
void DebuggerBackend::initializeCommandState<StepCommand>(const StepCommand& cmd) {
    commandState = StepCommandState();
}

// MicroStep
template <>
void DebuggerBackend::initializeCommandState<MicroStepCommand>(const MicroStepCommand& cmd) {
    commandState = MicroStepCommandState();
}

// Next
template <>
void DebuggerBackend::initializeCommandState<NextCommand>(const NextCommand& cmd) {
    commandState = NextCommandState {0, core.gb.cpu.SP};
}

// MicroNext
template <>
void DebuggerBackend::initializeCommandState<MicroNextCommand>(const MicroNextCommand& cmd) {
    commandState = MicroNextCommandState {0, core.gb.cpu.SP};
}

// Frame
template <>
void DebuggerBackend::initializeCommandState<FrameCommand>(const FrameCommand& cmd) {
    commandState = FrameCommandState {};
}

// FrameBack
template <>
void DebuggerBackend::initializeCommandState<FrameBackCommand>(const FrameBackCommand& cmd) {
    commandState = FrameBackCommandState {};
}

// Scanline
template <>
void DebuggerBackend::initializeCommandState<ScanlineCommand>(const ScanlineCommand& cmd) {
    commandState = ScanlineCommandState {};
}

// Continue
template <>
void DebuggerBackend::initializeCommandState<ContinueCommand>(const ContinueCommand& cmd) {
    commandState = ContinueCommandState();
}

// Abort
template <>
void DebuggerBackend::initializeCommandState<AbortCommand>(const AbortCommand& cmd) {
    commandState = AbortCommandState();
}

// ===================== COMMANDS IMPLEMENTATION ===============================

// Tick
template <>
void DebuggerBackend::handleCommand<TickCommand, TickCommandState>(const TickCommand& cmd, TickCommandState& state) {
    if (core.ticks >= state.target)
        pullCommand(ExecutionCompleted());
}

// Dot
template <>
void DebuggerBackend::handleCommand<DotCommand, DotCommandState>(const DotCommand& cmd, DotCommandState& state) {
    if (core.gb.ppu.cycles >= state.target)
        pullCommand(ExecutionCompleted());
}

// Step
template <>
void DebuggerBackend::handleCommand<StepCommand, StepCommandState>(const StepCommand& cmd, StepCommandState& state) {
    Cpu& cpu = core.gb.cpu;
    if (!DebuggerHelpers::isInIsr(cpu) && cpu.instruction.microop.counter == 0) {
        if (++state.counter >= cmd.count)
            pullCommand(ExecutionCompleted());
    }
}

// MicroStep
template <>
void DebuggerBackend::handleCommand<MicroStepCommand, MicroStepCommandState>(const MicroStepCommand& cmd,
                                                                             MicroStepCommandState& state) {
    if (++state.counter >= cmd.count)
        pullCommand(ExecutionCompleted());
}

// Next
template <>
void DebuggerBackend::handleCommand<NextCommand, NextCommandState>(const NextCommand& cmd, NextCommandState& state) {
    Cpu& cpu = core.gb.cpu;
    if (!DebuggerHelpers::isInIsr(cpu) && cpu.instruction.microop.counter == 0) {
        if (cpu.SP == state.stackLevel)
            ++state.counter;
        if (state.counter >= cmd.count || cpu.SP > state.stackLevel)
            pullCommand(ExecutionCompleted());
    }
}

// MicroNext
template <>
void DebuggerBackend::handleCommand<MicroNextCommand, MicroNextCommandState>(const MicroNextCommand& cmd,
                                                                             MicroNextCommandState& state) {
    Cpu& cpu = core.gb.cpu;
    if (cpu.SP == state.stackLevel)
        ++state.counter;
    if (state.counter >= cmd.count || cpu.SP > state.stackLevel)
        pullCommand(ExecutionCompleted());
}

// Frame
template <>
void DebuggerBackend::handleCommand<FrameCommand, FrameCommandState>(const FrameCommand& cmd,
                                                                     FrameCommandState& state) {
    bool isNewFrame = core.gb.ppu.tickSelector == &Ppu::vBlank && core.gb.video.LY == 144 && core.gb.ppu.dots == 0;
    if (isNewFrame) {
        if (++state.counter >= cmd.count)
            pullCommand(ExecutionCompleted());
    }
}

// FrameBack
template <>
void DebuggerBackend::handleCommand<FrameBackCommand, FrameBackCommandState>(const FrameBackCommand& cmd,
                                                                             FrameBackCommandState& state) {
    // Unwind the history stack until the specific frame
    for (uint32_t i = 0; i < cmd.count - 1 && history.size() > 1; i++) {
        history.pop_back();
    }

    if (history.empty())
        return;

    // Load the state
    core.loadState(history.back().data());
    history.pop_back();

    pullCommand(ExecutionCompleted());
}

// Scanline
template <>
void DebuggerBackend::handleCommand<ScanlineCommand, ScanlineCommandState>(const ScanlineCommand& cmd,
                                                                           ScanlineCommandState& state) {
    bool isNewLine = core.gb.ppu.on && core.gb.ppu.tickSelector == &Ppu::oamScanEven && core.gb.ppu.dots == 0;
    if (isNewLine) {
        if (++state.counter >= cmd.count)
            pullCommand(ExecutionCompleted());
    }
}

// Continue
template <>
void DebuggerBackend::handleCommand<ContinueCommand, ContinueCommandState>(const ContinueCommand& cmd,
                                                                           ContinueCommandState& state) {
    // nop
}

// Abort
template <>
void DebuggerBackend::handleCommand<AbortCommand, AbortCommandState>(const AbortCommand& cmd,
                                                                     AbortCommandState& state) {
    run = false;
}

template <typename CommandType, typename CommandStateType>
void DebuggerBackend::handleCommand() {
    handleCommand(std::get<CommandType>(*command), std::get<CommandStateType>(*commandState));
}
