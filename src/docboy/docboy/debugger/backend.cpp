#include "docboy/debugger/backend.h"

#include <algorithm>

#include "../../../extra/extra/cartridge/header.h"
#include "docboy/core/core.h"
#include "docboy/debugger/frontend.h"
#include "docboy/debugger/helpers.h"
#include "docboy/debugger/mnemonics.h"
#include "docboy/debugger/symparser.h"

#include "utils/io.h"
#include "utils/std.h"

namespace {
constexpr uint32_t MAX_HISTORY_SIZE = 600; // ~10 sec
constexpr uint32_t MAX_CALLSTACK_SIZE = 8192;

template <typename T>
struct CommandTraits;

struct TCycleCommandTraits {
    static constexpr bool AllowExecutionOnTCycle = true;
};

struct MCycleCommandTraits {
    static constexpr bool AllowExecutionOnTCycle = false;
};

template <>
struct CommandTraits<TickCommand> : TCycleCommandTraits {
    using StateType = TickCommandState;
};

template <>
struct CommandTraits<DotCommand> : TCycleCommandTraits {
    using StateType = DotCommandState;
};

template <>
struct CommandTraits<StepCommand> : MCycleCommandTraits {
    using StateType = StepCommandState;
};

template <>
struct CommandTraits<MicroStepCommand> : MCycleCommandTraits {
    using StateType = MicroStepCommandState;
};

template <>
struct CommandTraits<NextCommand> : MCycleCommandTraits {
    using StateType = NextCommandState;
};

template <>
struct CommandTraits<MicroNextCommand> : MCycleCommandTraits {
    using StateType = MicroNextCommandState;
};

template <>
struct CommandTraits<FrameCommand> : TCycleCommandTraits {
    using StateType = FrameCommandState;
};

template <>
struct CommandTraits<FrameBackCommand> : TCycleCommandTraits {
    using StateType = FrameBackCommandState;
};

template <>
struct CommandTraits<ScanlineCommand> : TCycleCommandTraits {
    using StateType = ScanlineCommandState;
};

template <>
struct CommandTraits<ContinueCommand> : MCycleCommandTraits {
    using StateType = ContinueCommandState;
};

template <>
struct CommandTraits<AbortCommand> : MCycleCommandTraits {
    using StateType = AbortCommandState;
};

uint32_t hash_combine(uint32_t h1, uint32_t h2) {
    return h1 + h2 * 0xDEECE66D;
}
} // namespace

DebuggerBackend::DebuggerBackend(Core& core_) :
    core {core_} {
}

// ===================== COMMANDS STATE INITIALIZATION =========================

// Tick
template <>
void DebuggerBackend::init_command_state<TickCommand>(const TickCommand& cmd) {
    command_state = TickCommandState {core.ticks + cmd.count};
}

// Dot
template <>
void DebuggerBackend::init_command_state<DotCommand>(const DotCommand& cmd) {
    command_state = DotCommandState {core.gb.ppu.cycles + cmd.count};
}

// Step
template <>
void DebuggerBackend::init_command_state<StepCommand>(const StepCommand& cmd) {
    command_state = StepCommandState();
}

// MicroStep
template <>
void DebuggerBackend::init_command_state<MicroStepCommand>(const MicroStepCommand& cmd) {
    command_state = MicroStepCommandState();
}

// Next
template <>
void DebuggerBackend::init_command_state<NextCommand>(const NextCommand& cmd) {
    command_state = NextCommandState {0, core.gb.cpu.sp};
}

// MicroNext
template <>
void DebuggerBackend::init_command_state<MicroNextCommand>(const MicroNextCommand& cmd) {
    command_state = MicroNextCommandState {0, core.gb.cpu.sp};
}

// Frame
template <>
void DebuggerBackend::init_command_state<FrameCommand>(const FrameCommand& cmd) {
    command_state = FrameCommandState {};
}

// FrameBack
template <>
void DebuggerBackend::init_command_state<FrameBackCommand>(const FrameBackCommand& cmd) {
    command_state = FrameBackCommandState {};
}

// Scanline
template <>
void DebuggerBackend::init_command_state<ScanlineCommand>(const ScanlineCommand& cmd) {
    command_state = ScanlineCommandState {};
}

// Continue
template <>
void DebuggerBackend::init_command_state<ContinueCommand>(const ContinueCommand& cmd) {
    command_state = ContinueCommandState();
}

// Abort
template <>
void DebuggerBackend::init_command_state<AbortCommand>(const AbortCommand& cmd) {
    command_state = AbortCommandState();
}

// ===================== COMMANDS IMPLEMENTATION ===============================

// Tick
template <>
void DebuggerBackend::handle_command<TickCommand, TickCommandState>(const TickCommand& cmd, TickCommandState& state) {
    if (core.ticks >= state.target) {
        pull_command(ExecutionCompleted());
    }
}

// Dot
template <>
void DebuggerBackend::handle_command<DotCommand, DotCommandState>(const DotCommand& cmd, DotCommandState& state) {
    if (core.gb.ppu.cycles >= state.target) {
        pull_command(ExecutionCompleted());
    }
}

// Step
template <>
void DebuggerBackend::handle_command<StepCommand, StepCommandState>(const StepCommand& cmd, StepCommandState& state) {
    Cpu& cpu = core.gb.cpu;
    if (!DebuggerHelpers::is_in_isr(cpu) && cpu.instruction.microop.counter == 0) {
        if (++state.counter >= cmd.count) {
            pull_command(ExecutionCompleted());
        }
    }
}

// MicroStep
template <>
void DebuggerBackend::handle_command<MicroStepCommand, MicroStepCommandState>(const MicroStepCommand& cmd,
                                                                              MicroStepCommandState& state) {
    if (++state.counter >= cmd.count) {
        pull_command(ExecutionCompleted());
    }
}

// Next
template <>
void DebuggerBackend::handle_command<NextCommand, NextCommandState>(const NextCommand& cmd, NextCommandState& state) {
    Cpu& cpu = core.gb.cpu;
    if (!DebuggerHelpers::is_in_isr(cpu) && cpu.instruction.microop.counter == 0) {
        if (cpu.sp == state.stack_level) {
            ++state.counter;
        }
        if (state.counter >= cmd.count || cpu.sp > state.stack_level) {
            pull_command(ExecutionCompleted());
        }
    }
}

// MicroNext
template <>
void DebuggerBackend::handle_command<MicroNextCommand, MicroNextCommandState>(const MicroNextCommand& cmd,
                                                                              MicroNextCommandState& state) {
    Cpu& cpu = core.gb.cpu;
    if (cpu.sp == state.stack_level) {
        ++state.counter;
    }
    if (state.counter >= cmd.count || cpu.sp > state.stack_level) {
        pull_command(ExecutionCompleted());
    }
}

// Frame
template <>
void DebuggerBackend::handle_command<FrameCommand, FrameCommandState>(const FrameCommand& cmd,
                                                                      FrameCommandState& state) {
    bool is_new_frame = core.gb.ppu.tick_selector == &Ppu::vblank && core.gb.ppu.ly == 144 && core.gb.ppu.dots == 0;
    if (is_new_frame) {
        if (++state.counter >= cmd.count) {
            pull_command(ExecutionCompleted());
        }
    }
}

// FrameBack
template <>
void DebuggerBackend::handle_command<FrameBackCommand, FrameBackCommandState>(const FrameBackCommand& cmd,
                                                                              FrameBackCommandState& state) {
    // Unwind the history stack until the specific frame
    for (uint32_t i = 0; i < cmd.count - 1 && history.size() > 1; i++) {
        history.pop_back();
    }

    if (history.empty()) {
        return;
    }

    // Load the state
    core.load_state(history.back().data());
    history.pop_back();

    pull_command(ExecutionCompleted());
}

// Scanline
template <>
void DebuggerBackend::handle_command<ScanlineCommand, ScanlineCommandState>(const ScanlineCommand& cmd,
                                                                            ScanlineCommandState& state) {
    bool is_new_line =
        core.gb.ppu.lcdc.enable && core.gb.ppu.tick_selector == &Ppu::oam_scan_even && core.gb.ppu.dots == 0;
    if (is_new_line) {
        if (++state.counter >= cmd.count) {
            pull_command(ExecutionCompleted());
        }
    }
}

// Continue
template <>
void DebuggerBackend::handle_command<ContinueCommand, ContinueCommandState>(const ContinueCommand& cmd,
                                                                            ContinueCommandState& state) {
    // nop
}

// Abort
template <>
void DebuggerBackend::handle_command<AbortCommand, AbortCommandState>(const AbortCommand& cmd,
                                                                      AbortCommandState& state) {
    run = false;
}

void DebuggerBackend::attach_frontend(DebuggerFrontend& frontend_) {
    frontend = &frontend_;
}

void DebuggerBackend::load_symbols(const std::string& path) {
    // TODO: handle prefix of address (<BANK> or BOOT)
    auto symbol_list = DebugSymbolsParser::parse_sym_file(path);

    for (auto& symbol : symbol_list) {
        symbols.emplace(symbol.address, std::move(symbol));
    }
}

std::optional<DebugSymbol> DebuggerBackend::get_symbol(uint16_t addr) const {
    if (symbols.find(addr) != symbols.end()) {
        return symbols.at(addr);
    }

    return std::nullopt;
}

const std::map<uint16_t, DebugSymbol>& DebuggerBackend::get_symbols() const {
    return symbols;
}

void DebuggerBackend::notify_tick(uint64_t tick) {
    if (!frontend) {
        return;
    }

    Cpu& cpu = core.gb.cpu;

    // Notify the frontend for extra actions to take (e.g. handle CTRL+C, tracing)
    frontend->notify_tick(tick);

    if (interrupted) {
        // User interruption (CTRL+C): pull command again
        interrupted = false;
        pull_command(ExecutionInterrupted());
        return;
    }

    // Cache the state at each new frame to allow rewinding with FrameBack command
    bool is_new_frame = core.gb.ppu.tick_selector == &Ppu::vblank && core.gb.ppu.ly == 144 && core.gb.ppu.dots == 0;
    if (is_new_frame) {
        // Eventually make space for the new state
        if (history.size() == MAX_HISTORY_SIZE) {
            history.pop_front();
        }

        // Save state and push it to the history
        std::vector<uint8_t> state;
        state.resize(core.get_state_size());
        core.save_state(state.data());

        history.push_back(std::move(state));
        ASSERT(history.size() <= MAX_HISTORY_SIZE);
    }

    bool is_aligned_with_instruction = (tick % 4) == 0;
#ifdef ENABLE_CGB
    is_aligned_with_instruction =
        is_aligned_with_instruction || (tick % 2 == 0 && core.gb.speed_switch_controller.is_double_speed_mode());
#endif

    // Handle new CPU instruction (check breakpoints, call stack, ...)
    if (is_aligned_with_instruction) {
        if (!DebuggerHelpers::is_in_isr(cpu) && cpu.instruction.microop.counter == 0) {
            if (last_instruction) {
                const uint8_t last_opcode = last_instruction->instruction[0];
                // Check whether PC is changed due to a CALL
                if (last_opcode == 0xC4 || last_opcode == 0xCC || last_opcode == 0xCD || last_opcode == 0xD4 ||
                    last_opcode == 0xDC) {
                    ASSERT(last_instruction->instruction.size() == 3);
                    const uint16_t last_call_target_PC =
                        concat(last_instruction->instruction[2], last_instruction->instruction[1]);
                    if (cpu.instruction.address == last_call_target_PC) {
                        // We are at PC given by the last CALL instruction: push a call stack entry
                        ASSERT(call_stack.size() < MAX_CALLSTACK_SIZE);
                        call_stack.emplace_back(*last_instruction);
                    }
                }
                // Check whether PC is changed due to a RET
                else if (!call_stack.empty() && (last_opcode == 0xC0 || last_opcode == 0xC8 || last_opcode == 0xC9 ||
                                                 last_opcode == 0xD0 || last_opcode == 0xD8 || last_opcode == 0xD9)) {
                    const auto& last_call = call_stack.back();
                    if (cpu.instruction.address == last_call.address + 3) {
                        // We are at PC of the last CALL instruction: pop a call stack entry
                        call_stack.pop_back();
                    }
                }
            }

            // Disassemble the current instruction
            last_instruction = disassemble(cpu.instruction.address, true);

            // Check breakpoints
            auto b = get_breakpoint(cpu.instruction.address);
            if (b) {
                // A breakpoint has been triggered: pull command again
                pull_command(ExecutionBreakpointHit {*b});
                return;
            }
        }
    }

    // Check watchpoints
    if (watchpoint_hit) {
        // Fill the missing watchpoint info (new value)
        watchpoint_hit->new_value = read_memory_raw(watchpoint_hit->address);

        // Check if watchpoint's conditions are actually satisfied
        const auto& watchpoint = watchpoint_hit->watchpoint;

        if ((watchpoint_hit->access_type == WatchpointHit::AccessType::Read &&
             (!watchpoint.condition.enabled ||
              (watchpoint.condition.condition.operation == Watchpoint::Condition::Operator::Equal &&
               watchpoint_hit->new_value == watchpoint.condition.condition.operand))) ||
            (watchpoint_hit->access_type == WatchpointHit::AccessType::Write &&

             ((watchpoint.type == Watchpoint::Type::ReadWrite || watchpoint.type == Watchpoint::Type::Write ||
               (watchpoint.type == Watchpoint::Type::Change &&
                watchpoint_hit->old_value != watchpoint_hit->new_value)) &&
              (!watchpoint.condition.enabled ||
               (watchpoint.condition.condition.operation == Watchpoint::Condition::Operator::Equal &&
                watchpoint_hit->new_value == watchpoint.condition.condition.operand))))) {

            // A watchpoint has been triggered: pull the command again
            ExecutionWatchpointHit state {*watchpoint_hit};
            state.watchpoint_hit.new_value = read_memory_raw(watchpoint_hit->address);
            pull_command(state);
        }

        // Reset watchpoint anyway
        watchpoint_hit = std::nullopt;
        return;
    }

    if (!command) {
        // No command yet: pull command
        pull_command(ExecutionCompleted());
        return;
    }

    // Handle current command
    std::visit(
        [this, is_aligned_with_instruction](auto&& cmd) {
            using CmdType = std::decay_t<decltype(cmd)>;
            using CmdTraits = CommandTraits<CmdType>;
            using CmdStateType = typename CmdTraits::StateType;
            if (is_aligned_with_instruction || CmdTraits::AllowExecutionOnTCycle) {
                handle_command<CmdType, CmdStateType>(cmd, std::get<CmdStateType>(*command_state));
            }
        },
        *command);
}

void DebuggerBackend::reset() {
    clear();
    core.reset();
}

bool DebuggerBackend::save_state(const std::string& path) const {
    std::vector<uint8_t> data(core.get_state_size());
    core.save_state(data.data());

    bool ok;
    write_file(path, data.data(), data.size(), &ok);

    return ok;
}

bool DebuggerBackend::load_state(const std::string& path) {
    bool ok;
    std::vector<uint8_t> data = read_file(path, &ok);

    if (data.size() != core.get_state_size()) {
        return false;
    }

    clear();
    core.load_state(data.data());
    return true;
}

void DebuggerBackend::notify_memory_read(uint16_t address) {
    if (!has_watchpoint(address)) {
        return;
    }

    if (!allow_memory_callbacks) {
        return;
    }

    if (watchpoint_hit) {
        // At the moment we handle at maximum one watchpoint hit per tick.
        return;
    }

    auto w = get_watchpoint(address);
    ASSERT(w);

    uint8_t current_value = read_memory_raw(address);
    if ((w->type == Watchpoint::Type::Read || w->type == Watchpoint::Type::ReadWrite)) {
        watchpoint_hit = WatchpointHit();
        watchpoint_hit->watchpoint = *w;
        watchpoint_hit->address = address;
        watchpoint_hit->access_type = WatchpointHit::AccessType::Read;
        watchpoint_hit->old_value = current_value;
    }
}

void DebuggerBackend::notify_memory_write(uint16_t address) {
    // Update the memory hash after a change so that we don't
    // have to recompute it from scratch each time.
    memory_hash = hash_combine(memory_hash, address);

    if (!has_watchpoint(address)) {
        return;
    }

    if (!allow_memory_callbacks) {
        return;
    }

    if (watchpoint_hit) {
        // At the moment we handle at maximum one watchpoint hit per tick.
        return;
    }

    auto w = get_watchpoint(address);
    ASSERT(w);

    if (((w->type == Watchpoint::Type::ReadWrite || w->type == Watchpoint::Type::Write) ||
         (w->type == Watchpoint::Type::Change))) {
        watchpoint_hit = WatchpointHit();
        watchpoint_hit->watchpoint = *w;
        watchpoint_hit->address = address;
        watchpoint_hit->access_type = WatchpointHit::AccessType::Write;
        watchpoint_hit->old_value = read_memory_raw(address);
    }
}

void DebuggerBackend::set_on_reset_callback(std::function<void()>&& callback) {
    on_reset_callback = std::move(callback);
}

bool DebuggerBackend::is_asking_to_quit() const {
    return !run;
}

const CartridgeInfo& DebuggerBackend::get_cartridge_info() {
    if (!cartridge_info) {
        cartridge_info = CartridgeInfo {};
        ASSERT(core.gb.cartridge_slot.cartridge);

        // Deduce cartridge info.
        const CartridgeHeader& header = core.gb.cartridge_header;
        const uint8_t* rom_data = core.gb.cartridge_slot.cartridge->get_rom_data();
        const uint32_t rom_size = core.gb.cartridge_slot.cartridge->get_rom_size();

        using namespace Specs::Cartridge::Header;

        if (rom_size < MemoryLayout::SIZE) {
            FATAL("unexpected rom size");
        }

        cartridge_info->title = CartridgeHeaderHelpers::title_as_string(header);
        cartridge_info->mbc = header.cartridge_type;
        cartridge_info->rom = header.rom_size;
        cartridge_info->ram = header.ram_size;
        cartridge_info->multicart = false;

        // All the known multicart are MBC1 of 1 MB.
        if (rom_size >= 0x100000 &&
            (cartridge_info->mbc == Mbc::MBC1 || cartridge_info->mbc == Mbc::MBC1_RAM ||
             cartridge_info->mbc == Mbc::MBC1_RAM_BATTERY) &&
            cartridge_info->rom == Rom::MB_1) {

            // Count the occurrences of Nintendo Logo.
            uint8_t num_logo {};
            for (uint8_t i = 0; i < 4; i++) {
                num_logo += memcmp(rom_data + (0x40000 * i) + MemoryLayout::LOGO::START, NINTENDO_LOGO,
                                   sizeof(NINTENDO_LOGO)) == 0;
            }

            cartridge_info->multicart = num_logo > 1;
        }
    }

    return *cartridge_info;
}

uint32_t DebuggerBackend::add_breakpoint(uint16_t addr) {
    const uint32_t id = next_point_id++;
    const Breakpoint b {id, addr};
    breakpoints.push_back(b);
    return id;
}

std::optional<Breakpoint> DebuggerBackend::get_breakpoint(uint16_t addr) const {
    if (const auto b = std::find_if(breakpoints.begin(), breakpoints.end(),
                                    [addr](const Breakpoint& b) {
                                        return b.address == addr;
                                    });
        b != breakpoints.end()) {
        return *b;
    }
    return std::nullopt;
}

const std::vector<Breakpoint>& DebuggerBackend::get_breakpoints() const {
    return breakpoints;
}

uint32_t DebuggerBackend::add_watchpoint(Watchpoint::Type type, uint16_t from, uint16_t to,
                                         std::optional<Watchpoint::Condition> cond) {
    const uint32_t id = next_point_id++;
    const Watchpoint w {id, type, {from, to}, {static_cast<bool>(cond), cond ? *cond : Watchpoint::Condition()}};
    watchpoints.push_back(w);
    for (uint16_t i = from; i <= to; i++) {
        watchpoints_at_address[i] += 1;
    }
    return id;
}

std::optional<Watchpoint> DebuggerBackend::get_watchpoint(uint16_t addr) const {
    if (const auto w = std::find_if(watchpoints.begin(), watchpoints.end(),
                                    [addr](const Watchpoint& wp) {
                                        return wp.address.from <= addr && addr <= wp.address.to;
                                    });
        w != watchpoints.end()) {
        ASSERT(has_watchpoint(addr));
        return *w;
    }

    ASSERT(!has_watchpoint(addr));

    return std::nullopt;
}

const std::vector<Watchpoint>& DebuggerBackend::get_watchpoints() const {
    return watchpoints;
}

bool DebuggerBackend::has_watchpoint(uint16_t addr) const {
    // More efficient get_watchpoint.
    return watchpoints_at_address[addr];
}

void DebuggerBackend::remove_point(uint32_t id) {
    erase_if(breakpoints, [id](const Breakpoint& b) {
        return b.id == id;
    });
    if (const auto w = std::find_if(watchpoints.begin(), watchpoints.end(),
                                    [id](const Watchpoint& w) {
                                        return w.id == id;
                                    });
        w != watchpoints.end()) {
        for (uint16_t i = w->address.from; i <= w->address.to; i++) {
            watchpoints_at_address[i] -= 1;
        }

        erase_if(watchpoints, [id](const Watchpoint& w) {
            return w.id == id;
        });
    }
}

void DebuggerBackend::clear_points() {
    breakpoints.clear();
    watchpoints.clear();
    memset(watchpoints_at_address, 0, sizeof(watchpoints_at_address));
}

std::optional<DisassembledInstructionReference> DebuggerBackend::disassemble(uint16_t addr, bool cache) {
    auto instrs = disassemble_multi(addr, 1, cache);
    if (instrs.empty()) {
        return {};
    }
    return instrs[0];
}

std::vector<DisassembledInstructionReference> DebuggerBackend::disassemble_multi(uint16_t addr, uint16_t n,
                                                                                 bool cache) {
    std::vector<DisassembledInstructionReference> refs;

    uint32_t address_cursor = addr;
    for (uint32_t i = 0; i < n && address_cursor <= 0xFFFF; i++) {
        auto instruction = do_disassemble(address_cursor);
        if (!instruction) {
            break;
        }
        if (cache) {
            disassembled_instructions[address_cursor] = instruction;
        }
        refs.emplace_back(address_cursor, *instruction);
        address_cursor += instruction->size();
    }

    return refs;
}

std::vector<DisassembledInstructionReference> DebuggerBackend::disassemble_range(uint16_t from, uint16_t to,
                                                                                 bool cache) {
    std::vector<DisassembledInstructionReference> refs;

    for (uint32_t address_cursor = from; address_cursor <= to && address_cursor <= 0xFFFF;) {
        auto instruction = do_disassemble(address_cursor);
        if (!instruction) {
            break;
        }
        if (cache) {
            disassembled_instructions[address_cursor] = instruction;
        }
        refs.emplace_back(address_cursor, *instruction);
        address_cursor += instruction->size();
    }

    return refs;
}

std::optional<DisassembledInstruction> DebuggerBackend::get_disassembled_instruction(uint16_t addr) const {
    return disassembled_instructions[addr];
}

std::vector<DisassembledInstructionReference> DebuggerBackend::get_disassembled_instructions() const {
    std::vector<DisassembledInstructionReference> refs;
    for (uint32_t addr = 0; addr < array_size(disassembled_instructions); addr++) {
        if (disassembled_instructions[addr]) {
            refs.emplace_back(addr, *disassembled_instructions[addr]);
        }
    }
    return refs;
}

uint8_t DebuggerBackend::read_memory(uint16_t addr) {
    allow_memory_callbacks = false;
    uint8_t value = DebuggerHelpers::read_memory(core.gb.mmu, addr);
    allow_memory_callbacks = true;
    return value;
}

uint8_t DebuggerBackend::read_memory_raw(uint16_t addr) {
    allow_memory_callbacks = false;
    uint8_t value = DebuggerHelpers::read_memory_raw(core.gb, addr);
    allow_memory_callbacks = true;
    return value;
}

std::optional<DisassembledInstruction> DebuggerBackend::do_disassemble(uint16_t addr) {
    uint32_t address_cursor = addr;
    uint8_t opcode = read_memory(address_cursor++);
    DisassembledInstruction instruction {opcode};

    // this works for CB and non CB because all CB instructions have length 2
    uint8_t length = instruction_length(opcode);

    while (address_cursor < addr + length && address_cursor <= 0xFFFF)
        instruction.push_back(read_memory(address_cursor++));

    return instruction;
}

void DebuggerBackend::pull_command(const ExecutionState& state) {
    command = frontend->pull_command(state);
    std::visit(
        [this](auto&& cmd) {
            init_command_state(cmd);
        },
        *command);
}

void DebuggerBackend::interrupt() {
    interrupted = true;
}

const Core& DebuggerBackend::get_core() const {
    return core;
}

void DebuggerBackend::clear() {
    // Reset debugger state
    command = std::nullopt;
    command_state = std::nullopt;
    run = true;
    interrupted = false;
    cartridge_info = std::nullopt;
    breakpoints.clear();
    watchpoints.clear();
    memset(watchpoints_at_address, 0, sizeof(watchpoints_at_address));
    for (uint32_t i = 0; i < array_size(disassembled_instructions); i++) {
        disassembled_instructions[i] = std::nullopt;
    }
    watchpoint_hit = std::nullopt;
    next_point_id = 0;
    allow_memory_callbacks = true;
    history.clear();
    last_instruction = std::nullopt;
    call_stack.clear();
    memory_hash = 0;

    // Eventually notify observers
    if (on_reset_callback) {
        on_reset_callback();
    }
}

void DebuggerBackend::proceed() {
    command = ContinueCommand();
    init_command_state<ContinueCommand>(std::get<ContinueCommand>(*command));
}

const std::vector<DisassembledInstructionReference>& DebuggerBackend::get_call_stack() const {
    return call_stack;
}

uint32_t DebuggerBackend::state_hash() const {
    const auto& gb = core.gb;

    uint32_t h {};
    h = hash_combine(h, core.ticks);
    h = hash_combine(h, gb.cpu.af);
    h = hash_combine(h, gb.cpu.bc);
    h = hash_combine(h, gb.cpu.de);
    h = hash_combine(h, gb.cpu.hl);
    h = hash_combine(h, gb.cpu.pc);
    h = hash_combine(h, gb.cpu.sp);
    h = hash_combine(h, gb.interrupts.IE);
    h = hash_combine(h, gb.interrupts.IF);
    h = hash_combine(h, gb.timers.div16);
    h = hash_combine(h, gb.timers.tima);
    h = hash_combine(h, gb.timers.tma);
    h = hash_combine(h, gb.timers.tac.enable);
    h = hash_combine(h, gb.timers.tac.clock_selector);
    h = hash_combine(h, memory_hash);

    return h;
}