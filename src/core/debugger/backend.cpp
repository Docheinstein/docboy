#include "backend.h"
#include "core/core.h"
#include "core/helpers.h"
#include "gameboy.h"
#include "utils/arrayutils.h"
#include "utils/stdutils.h"
#include <algorithm>

DebuggerBackend::DebuggerBackend(ICoreDebug& core) :
    core(core),
    gameboy(core.getGameBoy()),
    frontend(),
    nextPointId(),
    commandState(),
    interrupted() {
    core.setObserver(this);
}

void DebuggerBackend::attachFrontend(IDebuggerFrontend* frontend_) {
    frontend = frontend_;
}

void DebuggerBackend::detachFrontend() {
    frontend = nullptr;
}

uint32_t DebuggerBackend::addBreakpoint(uint16_t addr) {
    uint32_t id = nextPointId++;
    Debugger::Breakpoint b = {.id = id, .address = addr};
    breakpoints.push_back(b);
    return id;
}

std::optional<Debugger::Breakpoint> DebuggerBackend::getBreakpoint(uint16_t addr) const {
    auto b = std::find_if(breakpoints.begin(), breakpoints.end(), [addr](const Debugger::Breakpoint& b) {
        return b.address == addr;
    });
    if (b != breakpoints.end())
        return *b;
    return std::nullopt;
}

const std::vector<Debugger::Breakpoint>& DebuggerBackend::getBreakpoints() const {
    return breakpoints;
}

uint32_t DebuggerBackend::addWatchpoint(Debugger::Watchpoint::Type type, uint16_t from, uint16_t to,
                                        std::optional<Debugger::Watchpoint::Condition> cond) {
    uint32_t id = nextPointId++;
    Debugger::Watchpoint w {
        .id = id,
        .type = type,
        .address = {.from = from, .to = to},
        .condition = {.enabled = (bool)cond, .condition = cond ? *cond : Debugger::Watchpoint::Condition()}};
    watchpoints.push_back(w);
    return id;
}

std::optional<Debugger::Watchpoint> DebuggerBackend::getWatchpoint(uint16_t addr) const {
    auto w = std::find_if(watchpoints.begin(), watchpoints.end(), [addr](const Debugger::Watchpoint& wp) {
        return wp.address.from <= addr && addr <= wp.address.to;
    });
    if (w != watchpoints.end())
        return *w;
    return std::nullopt;
}

const std::vector<Debugger::Watchpoint>& DebuggerBackend::getWatchpoints() const {
    return watchpoints;
}

void DebuggerBackend::removePoint(uint32_t id) {
    erase_if(breakpoints, [id](const Debugger::Breakpoint& b) {
        return b.id == id;
    });
    erase_if(watchpoints, [id](const Debugger::Watchpoint& w) {
        return w.id == id;
    });
}

void DebuggerBackend::clearPoints() {
    breakpoints.clear();
    watchpoints.clear();
}

void DebuggerBackend::disassemble(uint16_t addr, size_t n) {
    uint32_t addressCursor = addr;
    for (size_t i = 0; i < n && addressCursor <= 0xFFFF; i++) {
        auto instruction = doDisassemble(addressCursor);
        if (!instruction)
            break;
        disassembled[addressCursor] = instruction;
        addressCursor += instruction->size();
    }
}

void DebuggerBackend::disassembleRange(uint16_t from, uint16_t to) {
    for (uint32_t addressCursor = from; addressCursor <= to && addressCursor <= 0xFFFF;) {
        auto instruction = doDisassemble(addressCursor);
        if (!instruction)
            break;
        disassembled[addressCursor] = instruction;
        addressCursor += instruction->size();
    }
}

std::optional<Debugger::Disassemble> DebuggerBackend::doDisassemble(uint16_t addr) {
    uint32_t addressCursor = addr;
    uint8_t opcode = readMemory(addressCursor++);
    Debugger::Disassemble instruction {opcode};

    // this works for CB and non CB because all CB instructions have length 2
    uint8_t length = instruction_length(opcode);

    while (addressCursor < addr + length && addressCursor <= 0xFFFF)
        instruction.push_back(readMemory(addressCursor++));

    return instruction;
}

std::optional<Debugger::Disassemble> DebuggerBackend::getDisassembled(uint16_t addr) const {
    return disassembled[addr];
}

Debugger::CPUState DebuggerBackend::getCPUState() const {
    return gameboy.getCPUDebug().getState();
}

Debugger::PPUState DebuggerBackend::getPPUState() const {
    return gameboy.getPPUDebug().getState();
}

Debugger::LCDState DebuggerBackend::getLCDState() const {
    return gameboy.getLCDDebug().getState();
}

uint8_t DebuggerBackend::readMemory(uint16_t addr) {
    Observer* o = core.getObserver();
    core.setObserver(nullptr);
    uint8_t value = gameboy.getBus().read(addr);
    core.setObserver(o);
    return value;
}

void DebuggerBackend::onMemoryRead(uint16_t addr, uint8_t value) {
    auto w = getWatchpoint(addr);
    if (w) {
        if ((w->type == Debugger::Watchpoint::Type::Read || w->type == Debugger::Watchpoint::Type::ReadWrite) &&
            (!w->condition.enabled ||
             (w->condition.condition.operation == Debugger::Watchpoint::Condition::Operator::Equal &&
              value == w->condition.condition.operand))) {
            watchpointHit = Debugger::WatchpointHit();
            watchpointHit->watchpoint = *w;
            watchpointHit->address = addr;
            watchpointHit->accessType = Debugger::WatchpointHit::AccessType::Read;
            watchpointHit->oldValue = value;
            watchpointHit->newValue = value;
        }
    }
}

void DebuggerBackend::onMemoryReadError(uint16_t addr, const std::string& error) {
    memoryReadError = {addr, error};
}

void DebuggerBackend::onMemoryWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) {
    auto w = getWatchpoint(addr);
    if (w) {
        if (((w->type == Debugger::Watchpoint::Type::ReadWrite || w->type == Debugger::Watchpoint::Type::Write) ||
             (w->type == Debugger::Watchpoint::Type::Change && oldValue != newValue)) &&
            (!w->condition.enabled ||
             (w->condition.condition.operation == Debugger::Watchpoint::Condition::Operator::Equal &&
              newValue == w->condition.condition.operand))) {
            watchpointHit = Debugger::WatchpointHit();
            watchpointHit->watchpoint = *w;
            watchpointHit->address = addr;
            watchpointHit->accessType = Debugger::WatchpointHit::AccessType::Write;
            watchpointHit->oldValue = oldValue;
            watchpointHit->newValue = newValue;
        }
    }
}
void DebuggerBackend::onMemoryWriteError(uint16_t addr, const std::string& error) {
    memoryWriteError = {addr, error};
}

bool DebuggerBackend::onTick(uint64_t tick) {
    auto pullCommand = [&](const Debugger::ExecutionState& state) {
        command = this->frontend->pullCommand(state);
        commandState.counter = 0;
        if (std::holds_alternative<Debugger::Commands::Next>(*command)) {
            commandState.stackLevel = getCPUState().registers.SP;
        }
        if (std::holds_alternative<Debugger::Commands::MicroNext>(*command)) {
            commandState.stackLevel = getCPUState().registers.SP;
        }
        if (std::holds_alternative<Debugger::Commands::Dot>(*command)) {
            Debugger::Commands::Dot cmdDot = std::get<Debugger::Commands::Dot>(*command);
            commandState.target = getPPUState().ppu.cycles + cmdDot.count;
        }
    };

    if (!frontend)
        return true;

    Debugger::CPUState cpu = getCPUState();

    if (!cpu.cycles)
        return true; // do not stop on first fetch

    bool isNewFrame =
        gameboy.getPPUDebug().isNewFrame() && get_bit<Bits::LCD::LCDC::LCD_ENABLE>(readMemory(Registers::LCD::LCDC));

    if (isNewFrame) {
        State& stateToSave = lastFramesStates.states[lastFramesStates.cursor];
        stateToSave.clear();
        core.saveState(stateToSave);
        lastFramesStates.cursor = (lastFramesStates.cursor + 1) % array_size(lastFramesStates.states);
        lastFramesStates.effectiveLastFramesNumber++;
    }

    this->frontend->onTick(tick);

    if (interrupted) {
        interrupted = false;
        pullCommand(Debugger::ExecutionStates::Interrupted());
        return true;
    }

    if (memoryReadError) {
        Debugger::ExecutionState outcome = Debugger::ExecutionStates::MemoryReadError {*memoryReadError};
        memoryReadError = std::nullopt;
        pullCommand(outcome);
        return true;
    }

    if (memoryWriteError) {
        Debugger::ExecutionState outcome = Debugger::ExecutionStates::MemoryWriteError {*memoryWriteError};
        memoryWriteError = std::nullopt;
        pullCommand(outcome);
        return true;
    }

    bool isMcycle = (tick % 4) == 0;

    if (isMcycle) {
        if (!cpu.instruction.ISR && cpu.instruction.microop == 0) {
            disassemble(cpu.instruction.address, 1);
            auto b = getBreakpoint(cpu.instruction.address);
            if (b) {
                Debugger::ExecutionState outcome = Debugger::ExecutionStates::BreakpointHit {*b};
                pullCommand(outcome);
                return true;
            }
        }
    }

    if (watchpointHit) {
        Debugger::ExecutionState outcome = Debugger::ExecutionStates::WatchpointHit {*watchpointHit};
        watchpointHit = std::nullopt;
        pullCommand(outcome);
        return true;
    }

    if (!command) {
        pullCommand(Debugger::ExecutionStates::Completed());
        return true;
    }

    const Debugger::Command& cmd = *command;

    if (std::holds_alternative<Debugger::Commands::Dot>(cmd)) {
        if (gameboy.getPPUDebug().getState().ppu.cycles >= commandState.target)
            pullCommand(Debugger::ExecutionStates::Completed());
        return true;
    }

    if (std::holds_alternative<Debugger::Commands::FrameBack>(cmd)) {
        Debugger::Commands::FrameBack cmdFrameBack = std::get<Debugger::Commands::FrameBack>(cmd);
        uint64_t safeFrameBackCount = std::min(cmdFrameBack.count, lastFramesStates.effectiveLastFramesNumber);
        if (safeFrameBackCount > 0) {
            uint64_t stateToRestoreIndex =
                (static_cast<int>(lastFramesStates.cursor) - safeFrameBackCount + array_size(lastFramesStates.states)) %
                array_size(lastFramesStates.states);
            State& stateToLoad = lastFramesStates.states[stateToRestoreIndex];
            stateToLoad.rewind();
            core.loadState(stateToLoad);
            lastFramesStates.cursor = (stateToRestoreIndex + 1) % array_size(lastFramesStates.states);
            lastFramesStates.effectiveLastFramesNumber -= safeFrameBackCount;
            pullCommand(Debugger::ExecutionStates::Completed());
        }
        return true;
    }

    if (std::holds_alternative<Debugger::Commands::Frame>(cmd)) {
        if (isNewFrame) {
            Debugger::Commands::Frame cmdFrame = std::get<Debugger::Commands::Frame>(cmd);
            commandState.counter++;
            if (commandState.counter >= cmdFrame.count)
                pullCommand(Debugger::ExecutionStates::Completed());
        }
        return true;
    }

    if (isMcycle) {
        if (std::holds_alternative<Debugger::Commands::Step>(cmd)) {
            Debugger::Commands::Step cmdStep = std::get<Debugger::Commands::Step>(cmd);
            if (!cpu.instruction.ISR && cpu.instruction.microop == 0) {
                commandState.counter++;
                if (commandState.counter >= cmdStep.count)
                    pullCommand(Debugger::ExecutionStates::Completed());
            }
            return true;
        }
        if (std::holds_alternative<Debugger::Commands::MicroStep>(cmd)) {
            Debugger::Commands::MicroStep cmdMicroStep = std::get<Debugger::Commands::MicroStep>(cmd);
            commandState.counter++;
            if (commandState.counter >= cmdMicroStep.count)
                pullCommand(Debugger::ExecutionStates::Completed());
            return true;
        }
        if (std::holds_alternative<Debugger::Commands::Next>(cmd)) {
            Debugger::Commands::Next cmdStep = std::get<Debugger::Commands::Next>(cmd);
            if (!cpu.instruction.ISR && cpu.instruction.microop == 0) {
                if (cpu.registers.SP == commandState.stackLevel)
                    commandState.counter++;
                if (commandState.counter >= cmdStep.count || cpu.registers.SP > commandState.stackLevel)
                    pullCommand(Debugger::ExecutionStates::Completed());
            }
            return true;
        }
        if (std::holds_alternative<Debugger::Commands::MicroNext>(cmd)) {
            Debugger::Commands::MicroNext cmdMicroNext = std::get<Debugger::Commands::MicroNext>(cmd);
            if (cpu.registers.SP == commandState.stackLevel)
                commandState.counter++;
            if (commandState.counter >= cmdMicroNext.count || cpu.registers.SP > commandState.stackLevel)
                pullCommand(Debugger::ExecutionStates::Completed());
            return true;
        }
        if (std::holds_alternative<Debugger::Commands::Continue>(cmd))
            return true;
        if (std::holds_alternative<Debugger::Commands::Abort>(cmd))
            return false;
    }

    return true;
}

void DebuggerBackend::interrupt() {
    interrupted = true;
}
