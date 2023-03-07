#include <algorithm>
#include "debuggerbackend.h"
#include "debuggerfrontend.h"
#include "debuggablecore.h"
#include "core/helpers.h"

DebuggerBackend::DebuggerBackend(IDebuggableCore &core) :
        core(core),
        cpu(core.getCpu()),
        ppu(core.getPpu()),
        lcd(core.getLcd()),
        frontend(),
        nextPointId(),
        interrupted(),
        stepCount(),
        nextCount() {
    core.setObserver(this);
}

void DebuggerBackend::attachFrontend(IDebuggerFrontend *frontend_) {
    frontend = frontend_;
}

void DebuggerBackend::detachFrontend() {
    frontend = nullptr;
}

uint32_t DebuggerBackend::addBreakpoint(uint16_t addr) {
    uint32_t id = nextPointId++;
    Breakpoint b = {
        .id = id,
        .address = addr
    };
    breakpoints.push_back(b);
    return id;
}

std::optional<DebuggerBackend::Breakpoint> DebuggerBackend::getBreakpoint(uint16_t addr) const {
    auto b = std::find_if(breakpoints.begin(), breakpoints.end(),
                         [addr](const Breakpoint &b) {
        return b.address == addr;
    });
    if (b != breakpoints.end())
        return *b;
    return std::nullopt;
}

const std::vector<DebuggerBackend::Breakpoint> & DebuggerBackend::getBreakpoints() const {
    return breakpoints;
}

uint32_t DebuggerBackend::addWatchpoint(Watchpoint::Type type, uint16_t from, uint16_t to,
                                        std::optional<Watchpoint::Condition> cond) {
    uint32_t id = nextPointId++;
    Watchpoint w {
        .id = id,
        .type = type,
        .address = {
            .from = from,
            .to = to
        },
        .condition = {
            .enabled = (bool) cond,
            .condition = cond ? *cond : Condition<uint8_t>()
        }
    };
    watchpoints.push_back(w);
    return id;
}

std::optional<DebuggerBackend::Watchpoint> DebuggerBackend::getWatchpoint(uint16_t addr) const {
    auto w = std::find_if(watchpoints.begin(), watchpoints.end(),
                         [addr](const Watchpoint &wp) {
        return wp.address.from <= addr && addr <= wp.address.to;
    });
    if (w != watchpoints.end())
        return *w;
    return std::nullopt;
}

const std::vector<DebuggerBackend::Watchpoint> & DebuggerBackend::getWatchpoints() const {
    return watchpoints;
}


void DebuggerBackend::removePoint(uint32_t id) {
    std::erase_if(breakpoints, [id](const Breakpoint &b) { return b.id == id; });
    std::erase_if(watchpoints, [id](const Watchpoint &w) { return w.id == id; });
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

std::optional<DebuggerBackend::Disassemble> DebuggerBackend::doDisassemble(uint16_t addr) {
    uint32_t addressCursor = addr;
    uint8_t opcode = readMemory(addressCursor++);
    Disassemble instruction { opcode };

    // this works for CB and non CB because all CB instructions have length 2
    uint8_t length = instruction_length(opcode);

    while (addressCursor < addr + length && addressCursor <= 0xFFFF)
        instruction.push_back(readMemory(addressCursor++));

    return instruction;
}


std::optional<DebuggerBackend::Disassemble> DebuggerBackend::getDisassembled(uint16_t addr) const {
    return disassembled[addr];
}

DebuggerBackend::CPUState DebuggerBackend::getCpuState() const {
    return {
        .registers = cpu.getRegisters(),
        .instruction = cpu.getCurrentInstruction(),
        .IME = cpu.getIME(),
        .halted = cpu.isHalted(),
        .cycles = cpu.getCycles()
    };
}

DebuggerBackend::PPUState DebuggerBackend::getPpuState() const {
    return {
        .ppu = {
            .state = ppu.getPPUState(),
            .bgFifo = ppu.getBgFifo(),
            .objFifo = ppu.getObjFifo(),
            .dots = ppu.getDots()
        },
        .fetcher = {
            .state = ppu.getFetcherState(),
            .x8 = ppu.getFetcherX(),
            .y = ppu.getFetcherY(),
            .lastTilemapAddr = ppu.getFetcherLastTileMapAddr(),
            .lastTileAddr = ppu.getFetcherLastTileAddr(),
            .lastTileDataAddr = ppu.getFetcherLastTileDataAddr(),
            .dots = ppu.getFetcherDots()
        },
        .cycles = ppu.getCycles()
    };
}


IDebuggerBackend::LCDState DebuggerBackend::getLcdState() const {
    return {
        .x = lcd.getX(),
        .y = lcd.getY()
    };
}


uint8_t DebuggerBackend::readMemory(uint16_t addr) {
    core.setObserver(nullptr);
    uint8_t value = cpu.readMemoryThroughCPU(addr);
    core.setObserver(this);
    return value;
}

void DebuggerBackend::onMemoryRead(uint16_t addr, uint8_t value) {
    auto w = getWatchpoint(addr);
    if (w) {
        if ((w->type == DebuggerBackend::Watchpoint::Type::Read ||
                w->type == DebuggerBackend::Watchpoint::Type::ReadWrite) &&
            (!w->condition.enabled ||
                (w->condition.condition.operation == Watchpoint::Condition::Operator::Equal &&
                value == w->condition.condition.operand))) {
            watchpointHit = WatchpointHit();
            watchpointHit->watchpoint = *w;
            watchpointHit->address = addr;
            watchpointHit->accessType = DebuggerBackend::WatchpointHit::AccessType::Read;
            watchpointHit->oldValue = value;
            watchpointHit->newValue = value;
        }
    }
}

void DebuggerBackend::onMemoryWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) {
    auto w = getWatchpoint(addr);
    if (w) {
        if (((w->type == DebuggerBackend::Watchpoint::Type::ReadWrite ||
                    w->type == DebuggerBackend::Watchpoint::Type::Write) ||
                (w->type == DebuggerBackend::Watchpoint::Type::Change && oldValue != newValue))
            &&
            (!w->condition.enabled ||
                (w->condition.condition.operation == Watchpoint::Condition::Operator::Equal &&
                newValue == w->condition.condition.operand))) {
            watchpointHit = WatchpointHit();
            watchpointHit->watchpoint = *w;
            watchpointHit->address = addr;
            watchpointHit->accessType = DebuggerBackend::WatchpointHit::AccessType::Write;
            watchpointHit->oldValue = oldValue;
            watchpointHit->newValue = newValue;
        }
    }
}

bool DebuggerBackend::onTick(uint8_t clk) {
    if (!frontend)
        return true;

    if (!cpu.getCycles())
        return true; // do not stop on first fetch

    frontend->onTick();

    if (interrupted) {
        interrupted = false;
        command = frontend->pullCommand({ .state = DebuggerBackend::ExecutionState::State::Interrupted });
        return true;
    }

    const bool isMcycle = (clk % 4) == 0;
    auto instruction = cpu.getCurrentInstruction();

    if (isMcycle) {
        if (!instruction.ISR && instruction.microop == 0) {
            disassemble(instruction.address, 1);
            auto b = getBreakpoint(instruction.address);
            if (b) {
                ExecutionState outcome = {
                        .state = DebuggerBackend::ExecutionState::State::BreakpointHit,
                        .breakpointHit = {*b}
                };
                command = frontend->pullCommand(outcome);
                return true;
            }
        }
    }

    if (watchpointHit) {
        ExecutionState outcome = {
            .state = DebuggerBackend::ExecutionState::State::WatchpointHit,
            .watchpointHit = *watchpointHit
        };
        watchpointHit = std::nullopt;
        command = frontend->pullCommand(outcome);
        return true;
    }

    if (!command) {
        command = frontend->pullCommand({ .state = ExecutionState::State::Completed });
        return true;
    }

    const Command &cmd = *command;

    if (std::holds_alternative<CommandDot>(cmd)) {
        CommandDot cmdDot = std::get<CommandDot>(cmd);
        dotCount++;
        if (dotCount >= cmdDot.count) {
            dotCount = 0;
            command = frontend->pullCommand({.state = ExecutionState::State::Completed});
        }
        return true;
    }

    if (isMcycle) {
        if (std::holds_alternative<CommandStep>(cmd)) {
            CommandStep cmdStep = std::get<CommandStep>(cmd);
            stepCount++;
            if (stepCount >= cmdStep.count) {
                stepCount = 0;
                command = frontend->pullCommand({.state = ExecutionState::State::Completed});
            }
            return true;
        } else if (std::holds_alternative<CommandNext>(cmd)) {
            CommandNext cmdNext = std::get<CommandNext>(cmd);
            if (!instruction.ISR && instruction.microop == 0) {
                nextCount++;
                if (nextCount >= cmdNext.count) {
                    nextCount = 0;
                    command = frontend->pullCommand({.state = ExecutionState::State::Completed});
                }
            }
            return true;
        } else if (std::holds_alternative<CommandContinue>(cmd)) {
            return true;
        } else if (std::holds_alternative<CommandAbort>(cmd)) {
            return false;
        }
    }

    return true;
}

void DebuggerBackend::interrupt() {
    interrupted = true;
}
