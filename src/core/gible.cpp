#include "gible.h"
#include "log/log.h"
#include "utils/binutils.h"
#include "debugger/debuggerfrontend.h"
#include "instructions.h"
#include <vector>
#include <algorithm>
#include "memorymap.h"
#include "cartridgefactory.h"


template<typename T, size_t size>
void Gible::VectorMap<T, size>::set(size_t location, T value) {
    auto loc = locations.find(location);
    if (loc == locations.end()) {
        entries.push_back(value);
        locations[location] = entries.size() - 1;
    } else {
        entries[loc->second] = value;
    }
}

template<typename T, size_t size>
void Gible::VectorMap<T, size>::unset(size_t location) {
    auto loc = locations.find(location);
    if (loc != locations.end()) {
        entries.erase(entries.begin() + loc->second);
        locations.erase(location);
    }
}

template<typename T, size_t size>
bool Gible::VectorMap<T, size>::has(size_t location) const {
    return locations.find(location) != locations.end();
}

template<typename T, size_t size>
std::optional<T> Gible::VectorMap<T, size>::get(size_t location) const {
    auto loc = locations.find(location);
    if (loc != locations.end()) {
        return entries[loc->second];
    }
    return std::nullopt;
}

template<typename T, size_t size>
void Gible::VectorMap<T, size>::clear() {
    entries.clear();
    locations.clear();
}

Gible::Gible() :
        io(),
        bus(wram1, wram2, io, hram, ie),
        cpu(bus),
        debugger(),
        breakpoints(),
        watchpoints(),
        serialLink(),
        debuggerAbortRequest(),
        debuggerInterruptRequest(),
        nextPointId(),
        divCounter(),
        timaCounter() {
}

Gible::~Gible() {

}

bool Gible::loadROM(const std::string &rom) {
    DEBUG(1) << "Loading ROM: " << rom << std::endl;

    auto c = CartridgeFactory::makeCartridge(rom);
    if (!c)
        return false;
    cartridge = std::move(c);
    bus.attachCartridge(cartridge);

#if DEBUG_LEVEL >= 1
    auto h = cartridge->header();
    DEBUG(1)
        << "ROM loaded\n"
        << "---------------\n"
        << cartridge << "\n"
        << "---------------\n"
        << "Entry point: " << hex(h.entry_point) << "\n"
        << "Title: " << h.title << "\n"
        << "CGB flag: " << hex(h.cgb_flag) << "\n"
        << "New licensee code: " << hex(h.new_licensee_code) << "\n"
        << "SGB flag: " << hex(h.sgb_flag) << "\n"
        << "Cartridge type: " << hex(h.cartridge_type) << "\n"
        << "ROM size: " << hex(h.rom_size) << "\n"
        << "RAM size: " << hex(h.ram_size) << "\n"
        << "Destination code: " << hex(h.destination_code) << "\n"
        << "Old licensee code: " << hex(h.old_licensee_code) << "\n"
        << "ROM version number: " << hex(h.rom_version_number) << "\n"
        << "Header checksum: " << hex(h.header_checksum) << "\n"
        << "Global checksum: " << hex(h.global_checksum) << "\n"
        << "---------------" << std::endl;

#endif
    return true;
}

void Gible::start() {
    while (!debuggerAbortRequest) {
        if (debugger) {
            debugger->onFrontend();
        } else {
            continue_();
        }
    }
}

void Gible::attachDebugger(DebuggerFrontend *frontend) {
    debugger = frontend;
    bus.setObserver(this);
}

void Gible::detachDebugger() {
    debugger = nullptr;
    bus.unsetObserver();
}

void Gible::attachSerialLink(SerialLink *serial) {
    serialLink = serial;
}


uint32_t Gible::addBreakpoint(uint16_t addr) {
    uint32_t id = nextPointId++;
    breakpoints.push_back({ id, addr });
    return id;
}

std::optional<DebuggerBackend::Breakpoint> Gible::getBreakpoint(uint16_t addr) const {
    auto b = std::find_if(breakpoints.begin(), breakpoints.end(),
                         [addr](const Breakpoint &b) {
        return b.address == addr;
    });
    if (b != breakpoints.end())
        return *b;
    return std::nullopt;
}


const std::vector<DebuggerBackend::Breakpoint> &Gible::getBreakpoints() const {
    return breakpoints;
}

uint32_t Gible::addWatchpoint(DebuggerBackend::Watchpoint::Type type, uint16_t from, uint16_t to,
                              std::optional<Watchpoint::Condition> cond) {
    uint32_t id = nextPointId++;
    watchpoints.push_back({ id, type, from, to, cond});
    return id;
}

void Gible::removePoint(uint32_t id) {
    std::erase_if(breakpoints, [id](const Breakpoint &b) { return b.id == id; });
    std::erase_if(watchpoints, [id](const Watchpoint &w) { return w.id == id; });
}

void Gible::clearPoints() {
    breakpoints.clear();
    watchpoints.clear();
}


//
//void Gible::setBreakpoint(Breakpoint b) {
//    breakpoints.set(b.address, b);
//}
//
//void Gible::unsetBreakpoint(uint16_t addr) {
//    breakpoints.unset(addr);
//}
//
//bool Gible::hasBreakpoint(uint16_t addr) const {
//    return breakpoints.has(addr);
//}
//
//std::optional<DebuggerBackend::Breakpoint> Gible::getBreakpoint(uint16_t addr) const {
//    return breakpoints.get(addr);
//}
//
//const std::vector<DebuggerBackend::Breakpoint> &Gible::getBreakpoints() const {
//    return breakpoints.entries;
//}
//
//void Gible::setWatchpoint(Watchpoint w) {
//    watchpoints.push_back(w);
////    watchpoints.set(w.address, w);
//}

//void Gible::unsetWatchpoint(uint16_t addr) {
//    watchpoints.unset(addr);
//}
//
//bool Gible::hasWatchpoint(uint16_t addr) const {
//    return watchpoints.has(addr);
//}
//
//std::optional<DebuggerBackend::Watchpoint> Gible::getWatchpoint(uint16_t addr) const {
//    return watchpoints.get(addr);
//}


std::optional<DebuggerBackend::Watchpoint> Gible::getWatchpoint(uint16_t addr) const {
    auto w = std::find_if(watchpoints.begin(), watchpoints.end(),
                         [addr](const Watchpoint &wp) {
        return wp.from <= addr && addr <= wp.to;
    });
    if (w != watchpoints.end())
        return *w;
    return std::nullopt;
}


const std::vector<DebuggerBackend::Watchpoint> &Gible::getWatchpoints() const {
    return watchpoints;
//    return watchpoints.entries;
}


uint32_t Gible::addCyclepoint(uint64_t n) {
    uint32_t id = nextPointId++;
    cyclepoints.push_back({ id, n });
    return id;
}

std::optional<DebuggerBackend::Cyclepoint> Gible::getCyclepoint(uint64_t n) const {
    auto c = std::find_if(cyclepoints.begin(), cyclepoints.end(),
                         [n](const Cyclepoint &c) {
        return c.n == n;
    });
    if (c != cyclepoints.end())
        return *c;
    return std::nullopt;
}

const std::vector<DebuggerBackend::Cyclepoint> &Gible::getCyclepoints() const {
    return cyclepoints;
}



std::optional<DebuggerBackend::DisassembleEntry> Gible::getCode(uint16_t addr) const {
    return code.get(addr);
}

const std::vector<DebuggerBackend::DisassembleEntry> &Gible::getCode() const {
    return code.entries;
}


DebuggerBackend::ExecResult Gible::step() {
    return tick();
}

DebuggerBackend::ExecResult Gible::next() {
    ExecResult result {};
    do {
        result = tick();
    } while (
        cpu.getCurrentInstructionMicroOperation() != 0 &&
        result.reason == DebuggerBackend::ExecResult::EndReason::Completed);
    return result;
}

DebuggerBackend::ExecResult Gible::continue_() {
    ExecResult result {};
    do {
        result = next();

//        if (triggeredWatchpoint) {
//            TriggeredWatchpoint reason = *triggeredWatchpoint;
//            triggeredWatchpoint = std::nullopt;
//            return reason;
//        }
//
//        if (breakpoints.has(cpu.getCurrentInstructionAddress())) {
//            TriggeredBreakpoint reason {
//                .breakpoint = *breakpoints.get(cpu.getCurrentInstructionAddress())
//            };
//            return reason;
//        }
    } while (result.reason == DebuggerBackend::ExecResult::EndReason::Completed);
    return result;
}

DebuggerBackend::RegistersSnapshot Gible::getRegisters() {
    RegistersSnapshot snapshot {};
    snapshot.AF = cpu.getAF();
    snapshot.BC = cpu.getBC();
    snapshot.DE = cpu.getDE();
    snapshot.HL = cpu.getHL();
    snapshot.PC = cpu.getPC();
    snapshot.SP = cpu.getSP();
    return snapshot;
}

DebuggerBackend::FlagsSnapshot Gible::getFlags() {
    return {
        .Z = cpu.getZ(),
        .N = cpu.getN(),
        .H = cpu.getH(),
        .C = cpu.getC(),
    };
}

DebuggerBackend::InterruptsSnapshot Gible::getInterrupts() {
    return {
        .IME = cpu.getIME(),
        .IE = readMemory(MemoryMap::IE),
        .IF = readMemory(MemoryMap::IO::IF),
    };
}

void Gible::disassemble(uint16_t addr, size_t n) {
    bus.unsetObserver();
    for (size_t i = 0; i < n && addr <= 0xFFFF; i++) {
        auto entry = doDisassemble(addr);
        if (!entry)
            break;
        code.set(entry->address, *entry);
        addr += entry->instruction.size();
    }
    bus.setObserver(this);
}

void Gible::disassembleRange(uint16_t from, uint16_t to) {
    bus.unsetObserver();
    for (uint32_t addr = from; addr < to && addr <= 0xFFFF;) {
        auto entry = doDisassemble(addr);
        if (!entry)
            break;
        code.set(entry->address, *entry);
        addr += entry->instruction.size();
    }
    bus.setObserver(this);
}

std::optional<DebuggerBackend::DisassembleEntry> Gible::doDisassemble(uint16_t addr_) {
//    DEBUG(1) << "doDisassemble" << hex(addr_) << std::endl;
    uint32_t addr = addr_;
    DisassembleEntry entry;
    entry.address = addr;
    uint8_t instructionLength;
    uint8_t opcode = bus.read(addr++);

    entry.instruction.push_back(opcode);
     if (opcode == 0xCB) {
        if (addr > 0xFFFF)
            return std::nullopt;
        uint8_t opcodeCB = bus.read(addr++);
        instructionLength = INSTRUCTIONS_CB[opcodeCB].length;
        entry.instruction.push_back(opcodeCB);
    } else {
        instructionLength = INSTRUCTIONS[opcode].length;
    }

    while (addr < entry.address + instructionLength) {
        if (addr > 0xFFFF)
            return std::nullopt;
        uint8_t data = bus.read(addr++);
        entry.instruction.push_back(data);
    }
    return entry;
}

//void Gible::disassemble(uint16_t addr) {
//    return entry;
    /*
    std::vector<DisassembleEntry> result;
    for (uint32_t addr = from; addr <= to;) {
        DisassembleEntry entry;
        entry.address = addr;
        uint8_t opcode = bus.read(addr++);
        entry.instruction.push_back(opcode);

        if (opcode == 0xCB) {
            uint8_t opcodeCB = bus.read(addr++);
            entry.info = INSTRUCTIONS_CB[opcodeCB];
            entry.instruction.push_back(opcodeCB);
        }
        else {
            entry.info = INSTRUCTIONS[opcode];
        }

        while (addr < entry.address + entry.info.length) {
            uint8_t data = bus.read(addr++);
            entry.instruction.push_back(data);
        }

        result.push_back(entry);
        DEBUG(2) << "Disassembled entry at addr = " << hex(entry.address) << " " << entry.info.mnemonic << std::endl;
    }
    return result;
    */
//}


uint8_t Gible::readMemory(uint16_t addr) {
    bus.unsetObserver();
    uint8_t result = bus.read(addr);
    bus.setObserver(this);
    return result;
}


DebuggerBackend::Instruction Gible::getCurrentInstruction() {
    return {
        .address = cpu.getCurrentInstructionAddress(),
        .microop = cpu.getCurrentInstructionMicroOperation()
    };
}

void Gible::onBusRead(uint16_t addr, uint8_t value) {
    DEBUG(2) << "onBusRead(" << hex(addr) << ")" << std::endl;

    if (debugger) {
        auto w = getWatchpoint(addr);
        if (w) {
            if ((w->type == DebuggerBackend::Watchpoint::Type::Read ||
                    w->type == DebuggerBackend::Watchpoint::Type::ReadWrite) &&
                (!w->condition ||
                    (w->condition->operation == Watchpoint::Condition::Operator::Equal &&
                    value == w->condition->operand))) {
                triggeredWatchpoint = WatchpointTrigger();
                triggeredWatchpoint->watchpoint = *w;
                triggeredWatchpoint->address = addr;
                triggeredWatchpoint->type = DebuggerBackend::WatchpointTrigger::Type::Read;
                triggeredWatchpoint->oldValue = value;
                triggeredWatchpoint->newValue = value;
            }
        }
    }
}

void Gible::onBusWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) {
    DEBUG(2) << "onBusWrite(" << hex(addr) << ")" << std::endl;
    if (debugger) {
        auto w = getWatchpoint(addr);
        if (w) {
            if (((w->type == DebuggerBackend::Watchpoint::Type::ReadWrite ||
                    w->type == DebuggerBackend::Watchpoint::Type::Write) ||
                 (w->type == DebuggerBackend::Watchpoint::Type::Change &&
                    oldValue != newValue)) &&
                    (!w->condition ||
                    (w->condition->operation == Watchpoint::Condition::Operator::Equal &&
                    newValue == w->condition->operand))) {
                triggeredWatchpoint = WatchpointTrigger();
                triggeredWatchpoint->watchpoint = *w;
                triggeredWatchpoint->address = addr;
                triggeredWatchpoint->type = DebuggerBackend::WatchpointTrigger::Type::Write;
                triggeredWatchpoint->oldValue = oldValue;
                triggeredWatchpoint->newValue = newValue;
            }
        }
    }
}

DebuggerBackend::ExecResult Gible::tick() {
    if (debugger)
        debugger->onTick();
    if (debuggerInterruptRequest) {
        debuggerInterruptRequest = false;
        return {.reason = DebuggerBackend::ExecResult::EndReason::Interrupted};
    }
    if (debuggerAbortRequest) {
        return {.reason = DebuggerBackend::ExecResult::EndReason::Aborted};
    }

    auto now = std::chrono::high_resolution_clock::now();

    // TODO: this is always one but the interrupt breaks this invariant, try to make the +1 invariant
    auto m = cpu.getCurrentMcycle();
    cpu.tick();
    auto mIncRaw = cpu.getCurrentMcycle() - m;
    uint64_t mInc = mIncRaw;
    if (mInc == 0)
        mInc = 1; // TODO: baaad: because of halt

    // TODO: bad
    constexpr int CPU_HZ = 4194304;
    constexpr int CPU_M_HZ = CPU_HZ / 4;
    constexpr int DIV_HZ = 16384;

    // DIV
    divCounter += mInc;
    if (divCounter >= CPU_M_HZ / DIV_HZ) {
        divCounter = 0;
        uint8_t DIV = bus.read(MemoryMap::IO::DIV);
        bus.write(MemoryMap::IO::DIV, DIV + 1);
    }

    // TIMA
    uint8_t TAC = bus.read(MemoryMap::IO::TAC);
    constexpr int TAC_SELECTOR_HZ[4] = {4096, 262144, 65536, 16384};
    if (get_bit<Bits::IO::TAC::ENABLE>(TAC))
        timaCounter += mInc;

    if (timaCounter >= CPU_M_HZ / TAC_SELECTOR_HZ[bitmasked<2>(TAC)]) {
        timaCounter = 0;
        uint8_t TIMA = bus.read(MemoryMap::IO::TIMA);
        auto [result, overflow] = sum_carry<7>(TIMA, 1);
        if (overflow) {
            TIMA = bus.read(MemoryMap::IO::TMA);
            bus.write(MemoryMap::IO::TIMA, TIMA);

            uint8_t IF = bus.read(MemoryMap::IO::IF);
            set_bit<Bits::IO::IF::TIMER>(IF, true);
            bus.write(MemoryMap::IO::IF, IF);
        } else {
            bus.write(MemoryMap::IO::TIMA, result);
        }
    }

#ifdef GAMEBOY_DOCTOR
    if ((cpu.getCurrentInstructionMicroOperation() == 0 &&
        !cpu.getCurrentInstructionCB()) || (
        cpu.getCurrentInstructionCB() &&
        cpu.getCurrentInstructionMicroOperation() == INSTRUCTIONS_CB[cpu.getCurrentInstructionOpcode()].duration.min - 1)) {
        auto AF = cpu.getAF();
        auto BC = cpu.getBC();
        auto DE = cpu.getDE();
        auto HL = cpu.getHL();
        auto SP = cpu.getSP();
        uint16_t PC = cpu.getPC() - 1;
//        std::cerr
//            << "A:" << hex(get_byte<1>(AF)) << " "
//            << "F:" << hex(get_byte<0>(AF)) << " "
//            << "B:" << hex(get_byte<1>(BC)) << " "
//            << "C:" << hex(get_byte<0>(BC)) << " "
//            << "D:" << hex(get_byte<1>(DE)) << " "
//            << "E:" << hex(get_byte<0>(DE)) << " "
//            << "H:" << hex(get_byte<1>(HL)) << " "
//            << "L:" << hex(get_byte<0>(HL)) << " "
//            << "SP:" << hex(SP) << " "
//            << "PC:" << hex(PC) << " "
//            << "PCMEM:" << hex(bus.read(PC)) << "," << hex(bus.read(PC + 1)) << "," << hex(bus.read(PC + 2)) << "," << hex(bus.read(PC + 3))
//            << std::endl;
        if (cpu.getCurrentCycle() > 1 && !cpu.hasPendingInterrupt())
            std::cerr
                << "A: " << hex(get_byte<1>(AF)) << " "
                << "F: " << hex(get_byte<0>(AF)) << " "
                << "B: " << hex(get_byte<1>(BC)) << " "
                << "C: " << hex(get_byte<0>(BC)) << " "
                << "D: " << hex(get_byte<1>(DE)) << " "
                << "E: " << hex(get_byte<0>(DE)) << " "
                << "H: " << hex(get_byte<1>(HL)) << " "
                << "L: " << hex(get_byte<0>(HL)) << " "
                << "SP: " << hex(SP) << " "
                << "PC: " << "00:" << hex(PC) << " ("
                << hex(bus.read(PC)) << " "
                << hex(bus.read(PC + 1)) << " "
                << hex(bus.read(PC + 2)) << " "
                << hex(bus.read(PC + 3)) << ")"
                << std::endl;
    }
#endif

    // TODO: is it appropriate to handle serial data here? or maybe in CPU?
    uint8_t SC = bus.read(MemoryMap::IO::SC);
    if (serialLink) {
        if (get_bit<7>(SC) && get_bit<0>(SC))
            serialLink->tick();
    }

    if (debugger) {
        // TODO: so bad
        uint8_t microp = cpu.getCurrentInstructionMicroOperation();
        bool cb = cpu.getCurrentInstructionCB();
        if (microp == 0 && !cb) {
            uint16_t instrAddress = cpu.getCurrentInstructionAddress();
            disassemble(instrAddress, 1);
            auto b = getBreakpoint(instrAddress);
            if (b) {
                return {
                    .reason = DebuggerBackend::ExecResult::EndReason::Breakpoint,
                    .breakpointTrigger = {*b}
                };
            }
        }

        auto c = getCyclepoint(cpu.getCurrentCycle());
        if (c) {
            return {
                .reason = DebuggerBackend::ExecResult::EndReason::Cyclepoint,
                .cyclepointTrigger = {*c}
            };
        }

        if (triggeredWatchpoint) {
            ExecResult result = {
                .reason = DebuggerBackend::ExecResult::EndReason::Watchpoint,
                .watchpointTrigger = *triggeredWatchpoint
            };
            triggeredWatchpoint = std::nullopt;
            return result;
        }
    }

    return { .reason = DebuggerBackend::ExecResult::EndReason::Completed };
}

void Gible::reset() {
    cpu.reset();
    wram1.reset();
    wram2.reset();
    io.reset();

//    code.clear();
//    breakpoints.clear();
//    watchpoints.clear();
//    nextPointId = 0;
}

uint8_t Gible::serialRead() {
    return bus.read(MemoryMap::IO::SB);
}

void Gible::serialWrite(uint8_t data) {
    bus.write(MemoryMap::IO::SB, data);
    uint8_t SC = bus.read(SC);
    set_bit<7>(SC, false);
    bus.write(MemoryMap::IO::SC, SC);
    // TODO: interrupt
}

uint64_t Gible::getCurrentCycle() {
    return cpu.getCurrentCycle();
}

uint64_t Gible::getCurrentMcycle() {
    return cpu.getCurrentMcycle();
}

void Gible::abort() {
    debuggerAbortRequest = true;
}

void Gible::interrupt() {
    debuggerInterruptRequest = true;
}
