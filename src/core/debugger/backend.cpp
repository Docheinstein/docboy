#include "backend.h"
#include "core/core.h"
#include "core/helpers.h"
#include "gameboy.h"
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
    Debugger::Watchpoint w{
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
    std::erase_if(breakpoints, [id](const Debugger::Breakpoint& b) {
        return b.id == id;
    });
    std::erase_if(watchpoints, [id](const Debugger::Watchpoint& w) {
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
    Debugger::Disassemble instruction{opcode};

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

bool DebuggerBackend::onTick(uint64_t tick) {
    auto pullCommand = [&](Debugger::ExecutionState state) {
        command = this->frontend->pullCommand(state);
        commandState.counter = 0;
        if (std::holds_alternative<Debugger::CommandNext>(*command)) {
            commandState.stackLevel = getCPUState().registers.SP;
        }
        if (std::holds_alternative<Debugger::CommandMicroNext>(*command)) {
            commandState.stackLevel = getCPUState().registers.SP;
        }
        if (std::holds_alternative<Debugger::CommandDot>(*command)) {
            Debugger::CommandDot cmdDot = std::get<Debugger::CommandDot>(*command);
            commandState.target = getPPUState().ppu.cycles + cmdDot.count;
        }
    };

    if (!frontend)
        return true;

    Debugger::CPUState cpu = gameboy.getCPUDebug().getState();

    if (!cpu.cycles)
        return true; // do not stop on first fetch

    this->frontend->onTick(tick);

    if (interrupted) {
        interrupted = false;
        pullCommand({.state = Debugger::ExecutionState::State::Interrupted});
        return true;
    }

    bool isMcycle = (tick % 4) == 0;

    if (isMcycle) {
        if (!cpu.instruction.ISR && cpu.instruction.microop == 0) {
            disassemble(cpu.instruction.address, 1);
            auto b = getBreakpoint(cpu.instruction.address);
            if (b) {
                Debugger::ExecutionState outcome = {.state = Debugger::ExecutionState::State::BreakpointHit,
                                                    .breakpointHit = {*b}};
                pullCommand(outcome);
                return true;
            }
        }
    }

    if (watchpointHit) {
        Debugger::ExecutionState outcome = {.state = Debugger::ExecutionState::State::WatchpointHit,
                                            .watchpointHit = *watchpointHit};
        watchpointHit = std::nullopt;
        pullCommand(outcome);
        return true;
    }

    if (!command) {
        pullCommand({.state = Debugger::ExecutionState::State::Completed});
        return true;
    }

    const Debugger::Command& cmd = *command;

    if (std::holds_alternative<Debugger::CommandDot>(cmd)) {
        if (gameboy.getPPUDebug().getState().ppu.cycles >= commandState.target)
            pullCommand({.state = Debugger::ExecutionState::State::Completed});
        return true;
    }

    if (isMcycle) {
        if (std::holds_alternative<Debugger::CommandStep>(cmd)) {
            Debugger::CommandStep cmdStep = std::get<Debugger::CommandStep>(cmd);
            if (!cpu.instruction.ISR && cpu.instruction.microop == 0) {
                commandState.counter++;
                if (commandState.counter >= cmdStep.count)
                    pullCommand({.state = Debugger::ExecutionState::State::Completed});
            }
            return true;
        } else if (std::holds_alternative<Debugger::CommandMicroStep>(cmd)) {
            Debugger::CommandMicroStep cmdMicroStep = std::get<Debugger::CommandMicroStep>(cmd);
            commandState.counter++;
            if (commandState.counter >= cmdMicroStep.count)
                pullCommand({.state = Debugger::ExecutionState::State::Completed});
            return true;
        } else if (std::holds_alternative<Debugger::CommandNext>(cmd)) {
            Debugger::CommandNext cmdStep = std::get<Debugger::CommandNext>(cmd);
            if (!cpu.instruction.ISR && cpu.instruction.microop == 0) {
                if (cpu.registers.SP == commandState.stackLevel)
                    commandState.counter++;
                if (commandState.counter >= cmdStep.count || cpu.registers.SP > commandState.stackLevel)
                    pullCommand({.state = Debugger::ExecutionState::State::Completed});
            }
            return true;
        } else if (std::holds_alternative<Debugger::CommandMicroNext>(cmd)) {
            Debugger::CommandMicroNext cmdMicroNext = std::get<Debugger::CommandMicroNext>(cmd);
            if (cpu.registers.SP == commandState.stackLevel)
                commandState.counter++;
            if (commandState.counter >= cmdMicroNext.count || cpu.registers.SP > commandState.stackLevel)
                pullCommand({.state = Debugger::ExecutionState::State::Completed});
            return true;
        } else if (std::holds_alternative<Debugger::CommandFrame>(cmd)) {
            Debugger::CommandFrame cmdFrame = std::get<Debugger::CommandFrame>(cmd);
            // TODO: don't like ppu.getDots() == 0
            Debugger::PPUState ppu = gameboy.getPPUDebug().getState();
            if (ppu.ppu.state == IPPUDebug::PPUState::VBlank && ppu.ppu.dots == 0 &&
                get_bit<Bits::LCD::LCDC::LCD_ENABLE>(readMemory(Registers::LCD::LCDC))) {
                commandState.counter++;
                if (commandState.counter >= cmdFrame.count)
                    pullCommand({.state = Debugger::ExecutionState::State::Completed});
            }
            return true;
        } else if (std::holds_alternative<Debugger::CommandContinue>(cmd)) {
            return true;
        } else if (std::holds_alternative<Debugger::CommandAbort>(cmd)) {
            return false;
        }
    }

    return true;
}

void DebuggerBackend::interrupt() {
    interrupted = true;
}
