#include <algorithm>
#include "debuggerbackend.h"
#include "debuggerfrontend.h"
#include "debuggablecore.h"
#include "debuggablebus.h"
#include "debuggablecpu.h"
#include "core/helpers.h"

DebuggerBackend::DebuggerBackend(IDebuggableCore &core) :
        cpu(core.getCpu()),
        frontend(),
        nextPointId(),
        interrupted() {
    core.setObserver(this);
    cpu.setObserver(this);
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

std::optional<DebuggerBackend::Disassemble> DebuggerBackend::doDisassemble(uint16_t addr) const {
    uint32_t addressCursor = addr;
    uint8_t opcode = cpu.readMemoryRaw(addressCursor++);
    Disassemble instruction { opcode };

    // this works for CB and non CB because all CB instructions have length 2
    uint8_t length = instruction_length(opcode);

    while (addressCursor < addr + length && addressCursor <= 0xFFFF)
        instruction.push_back(cpu.readMemoryRaw(addressCursor++));

    return instruction;
}


std::optional<DebuggerBackend::Disassemble> DebuggerBackend::getDisassembled(uint16_t addr) const {
    return disassembled[addr];
}

DebuggerBackend::CpuState DebuggerBackend::getCpuState() const {
    return {
        .registers = cpu.getRegisters(),
        .instruction = cpu.getCurrentInstruction(),
        .IME = cpu.getIME(),
        .halted = cpu.isHalted(),
        .cycles = cpu.getCycles()
    };
}


uint8_t DebuggerBackend::readMemory(uint16_t addr) {
    return cpu.readMemoryRaw(addr);
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

bool DebuggerBackend::onTick() {
    if (!frontend)
        return true;

    if (!cpu.getCycles())
        return true; // do not stop on nfirst fetch

    auto instruction = cpu.getCurrentInstruction();

    frontend->onTick();

    if (interrupted) {
        interrupted = false;
        command = frontend->pullCommand({ .state = DebuggerBackend::ExecutionState::State::Interrupted });
        return true;
    }

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

    switch (*command) {
    case Command::Step:
        command = frontend->pullCommand({ .state = ExecutionState::State::Completed });
        return true;
    case Command::Next:
        if (!instruction.ISR && instruction.microop == 0)
            command = frontend->pullCommand({ .state = ExecutionState::State::Completed });
        return true;
    case Command::Continue:
        return true;
    case Command::Abort:
        return false;
    }

    return true;
}

void DebuggerBackend::interrupt() {
    interrupted = true;
}
