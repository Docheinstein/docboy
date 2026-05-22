#include "docboy/debugger/backend.h"

#include <algorithm>

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

bool DebuggerBackend::BankedAddressMapKey::operator==(const BankedAddressMapKey& other) const {
    return bank == other.bank && address == other.address;
}

std::size_t DebuggerBackend::BankedAddressMapKey::Hash::operator()(const BankedAddressMapKey& key) const noexcept {
    return std::hash<uint32_t> {}(key.bank << 16 | key.address);
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
    symbols.list = DebugSymbolsParser::parse_sym_file(path);

    for (const auto& symbol : symbols.list) {
        symbols.by_address.emplace(BankedAddressMapKey {symbol.bank, symbol.address}, &symbol);
        symbols.by_name.emplace(symbol.name, &symbol);
    }
}

const DebugSymbol* DebuggerBackend::get_symbol(uint16_t bank, uint16_t addr) const {
    if (const auto sym = symbols.by_address.find({bank, addr}); sym != symbols.by_address.end()) {
        return sym->second;
    }

    return nullptr;
}

const DebugSymbol* DebuggerBackend::get_symbol(const std::string& name) const {
    if (const auto sym = symbols.by_name.find(name); sym != symbols.by_name.end()) {
        return sym->second;
    }

    return nullptr;
}

const std::vector<DebugSymbol>& DebuggerBackend::get_symbols() const {
    return symbols.list;
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

            // TODO: we should consider also banking (MBC mostly, but also WRAM, ...) while disassembling,
            //       otherwise code from different banks using the same address will be overwritten.
            // Disassemble the current instruction
            last_instruction = disassemble(std::nullopt, cpu.instruction.address, true);

            // Check breakpoints
            const Breakpoint* b = get_breakpoint(std::nullopt, cpu.instruction.address);
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
        watchpoint_hit->new_value = watchpoint_hit->watchpoint.raw
                                        ? read_memory_raw(std::nullopt, watchpoint_hit->address)
                                        : read_memory(watchpoint_hit->address);

        // Check if watchpoint's conditions are actually satisfied
        const auto& watchpoint = watchpoint_hit->watchpoint;

        if ((watchpoint_hit->access_type == WatchpointHit::AccessType::Read &&
             (!watchpoint.condition || (watchpoint.condition->operation == Watchpoint::Condition::Operator::Equal &&
                                        watchpoint_hit->new_value == watchpoint.condition->operand))) ||
            (watchpoint_hit->access_type == WatchpointHit::AccessType::Write &&

             ((watchpoint.type == Watchpoint::Type::ReadWrite || watchpoint.type == Watchpoint::Type::Write ||
               (watchpoint.type == Watchpoint::Type::Change &&
                watchpoint_hit->old_value != watchpoint_hit->new_value)) &&
              (!watchpoint.condition || (watchpoint.condition->operation == Watchpoint::Condition::Operator::Equal &&
                                         watchpoint_hit->new_value == watchpoint.condition->operand))))) {

            // A watchpoint has been triggered: pull the command again
            pull_command(ExecutionWatchpointHit {*watchpoint_hit});
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
    const Watchpoint* w = get_watchpoint(std::nullopt, address);

    if (!w) {
        return;
    }

    if (!allow_memory_callbacks) {
        return;
    }

    if (watchpoint_hit) {
        // At the moment we handle at maximum one watchpoint hit per tick.
        return;
    }

    if ((w->type == Watchpoint::Type::Read || w->type == Watchpoint::Type::ReadWrite)) {
        watchpoint_hit = WatchpointHit();
        watchpoint_hit->watchpoint = *w;
        watchpoint_hit->address = address;
        watchpoint_hit->access_type = WatchpointHit::AccessType::Read;
        watchpoint_hit->old_value = w->raw ? read_memory_raw(std::nullopt, address) : read_memory(address);
    }
}

void DebuggerBackend::notify_memory_write(uint16_t address) {
    // Update the memory hash after a change so that we don't
    // have to recompute it from scratch each time.
    memory_hash = hash_combine(memory_hash, address);

    const Watchpoint* w = get_watchpoint(std::nullopt, address);

    if (!w) {
        return;
    }

    if (!allow_memory_callbacks) {
        return;
    }

    if (watchpoint_hit) {
        // At the moment we handle at maximum one watchpoint hit per tick.
        return;
    }

    if (((w->type == Watchpoint::Type::ReadWrite || w->type == Watchpoint::Type::Write) ||
         (w->type == Watchpoint::Type::Change))) {
        watchpoint_hit = WatchpointHit();
        watchpoint_hit->watchpoint = *w;
        watchpoint_hit->address = address;
        watchpoint_hit->access_type = WatchpointHit::AccessType::Write;
        watchpoint_hit->old_value = w->raw ? read_memory_raw(std::nullopt, address) : read_memory(address);
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

        const auto title_as_string = [](const CartridgeHeader& header) {
            const auto* title_cstr = reinterpret_cast<const char*>(header.title);
            std::string title {title_cstr, strnlen(title_cstr, sizeof(header.title))};
#ifdef ENABLE_CGB
            if (title.size() == 16) {
                // Bit 15 is used for CGB flag instead of title for CGB-era cartridges.
                uint8_t flag = cgb_flag(header);
                if (flag == Specs::Cartridge::Header::CgbFlag::DMG_AND_CGB ||
                    flag == Specs::Cartridge::Header::CgbFlag::CGB_ONLY) {
                    title[15] = '\0';
                }
            }
#endif

            return title;
        };

        cartridge_info->title = title_as_string(header);
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

uint32_t DebuggerBackend::add_breakpoint(std::optional<uint16_t> bank, uint16_t addr) {
    const uint16_t actual_bank = bank_of(bank, addr);

    const uint32_t id = next_point_id++;
    const Breakpoint& breakpoint = breakpoints.list.emplace_back(Breakpoint {id, actual_bank, addr});
    breakpoints.by_address.emplace(BankedAddressMapKey {actual_bank, addr}, &breakpoint);
    return id;
}

const Breakpoint* DebuggerBackend::get_breakpoint(std::optional<uint16_t> bank, uint16_t addr) const {
    if (const auto it = breakpoints.by_address.find({bank_of(bank, addr), addr}); it != breakpoints.by_address.end()) {
        return it->second;
    }

    return nullptr;
}

const std::list<Breakpoint>& DebuggerBackend::get_breakpoints() const {
    return breakpoints.list;
}

uint32_t DebuggerBackend::add_watchpoint(Watchpoint::Type type, std::optional<uint16_t> bank, uint16_t from,
                                         uint16_t to, bool raw, std::optional<Watchpoint::Condition> cond) {
    const uint16_t actual_bank = bank_of(bank, from);

    const uint32_t id = next_point_id++;
    const Watchpoint& watchpoint =
        watchpoints.list.emplace_back(Watchpoint {id, type, actual_bank, {from, to}, raw, cond});

    for (uint16_t addr = from; addr <= to; addr++) {
        watchpoints.by_address.emplace(BankedAddressMapKey {actual_bank, addr}, &watchpoint);
    }

    return id;
}

const Watchpoint* DebuggerBackend::get_watchpoint(std::optional<uint16_t> bank, uint16_t addr) const {
    if (const auto it = watchpoints.by_address.find({bank_of(bank, addr), addr}); it != watchpoints.by_address.end()) {
        return it->second;
    }

    return nullptr;
}

const std::list<Watchpoint>& DebuggerBackend::get_watchpoints() const {
    return watchpoints.list;
}

void DebuggerBackend::remove_point(uint32_t id) {
    erase_if(breakpoints.by_address, [id](const std::pair<BankedAddressMapKey, const Breakpoint*>& addr_to_breakpoint) {
        return addr_to_breakpoint.second->id == id;
    });

    erase_if(breakpoints.list, [id](const Breakpoint& b) {
        return b.id == id;
    });

    erase_if(watchpoints.by_address, [id](const std::pair<BankedAddressMapKey, const Watchpoint*>& addr_to_watchpoint) {
        return addr_to_watchpoint.second->id == id;
    });

    erase_if(watchpoints.list, [id](const Watchpoint& w) {
        return w.id == id;
    });
}

void DebuggerBackend::clear_points() {
    breakpoints.list.clear();
    breakpoints.by_address.clear();
    watchpoints.list.clear();
    watchpoints.by_address.clear();
}

std::optional<DisassembledInstructionRef> DebuggerBackend::disassemble(std::optional<uint16_t> bank, uint16_t addr,
                                                                       bool cache) {
    if (auto instrs = disassemble_multi(bank, addr, 1, cache); !instrs.empty()) {
        return instrs[0];
    }

    return std::nullopt;
}

std::vector<DisassembledInstructionRef> DebuggerBackend::disassemble_multi(std::optional<uint16_t> bank, uint16_t addr,
                                                                           uint16_t n, bool cache) {
    std::vector<DisassembledInstructionRef> refs;

    uint32_t address_cursor = addr;
    for (uint32_t i = 0; i < n && address_cursor <= 0xFFFF; i++) {
        auto instruction = do_disassemble(bank, address_cursor);

        uint16_t actual_bank = bank_of(bank, address_cursor);

        if (cache) {
            if (!disassembled_instructions[actual_bank]) {
                disassembled_instructions[actual_bank] =
                    std::make_unique<std::array<std::optional<DisassembledInstruction>, 0x10000>>();
            }
            (*disassembled_instructions[actual_bank])[address_cursor] = instruction;
        }

        refs.emplace_back(actual_bank, address_cursor, instruction);
        address_cursor += instruction.size();
    }

    return refs;
}

std::vector<DisassembledInstructionRef> DebuggerBackend::disassemble_range(std::optional<uint16_t> bank, uint16_t from,
                                                                           uint16_t to, bool cache) {
    std::vector<DisassembledInstructionRef> refs;

    for (uint32_t address_cursor = from; address_cursor <= to && address_cursor <= 0xFFFF;) {
        auto instruction = do_disassemble(bank, address_cursor);

        uint16_t actual_bank = bank_of(bank, address_cursor);

        if (cache) {
            if (!disassembled_instructions[actual_bank]) {
                disassembled_instructions[actual_bank] =
                    std::make_unique<std::array<std::optional<DisassembledInstruction>, 0x10000>>();
            }
            (*disassembled_instructions[actual_bank])[address_cursor] = instruction;
        }

        refs.emplace_back(actual_bank, address_cursor, instruction);
        address_cursor += instruction.size();
    }

    return refs;
}

const DisassembledInstruction* DebuggerBackend::get_disassembled_instruction(uint16_t bank, uint16_t addr) const {
    if (const auto& disassembled_instructions_for_bank = disassembled_instructions[bank]) {
        if (const std::optional<DisassembledInstruction>& instruction = (*disassembled_instructions_for_bank)[addr]) {
            return &*instruction;
        }
    }

    return nullptr;
}

std::vector<DisassembledInstructionRef> DebuggerBackend::get_disassembled_instructions() const {
    std::vector<DisassembledInstructionRef> refs;

    // TODO: avoid copying all the instructions?
    for (uint32_t bank = 0; bank < array_size(disassembled_instructions); ++bank) {
        if (const auto& disassembled_instructions_for_bank = disassembled_instructions[bank]) {
            for (uint32_t addr = 0; addr < disassembled_instructions_for_bank->size(); addr++) {
                if (const std::optional<DisassembledInstruction>& instruction =
                        (*disassembled_instructions_for_bank)[addr]) {
                    refs.emplace_back(bank, addr, *instruction);
                }
            }
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

uint8_t DebuggerBackend::read_memory_raw(std::optional<uint16_t> bank, uint16_t addr) {
    allow_memory_callbacks = false;
    uint8_t value = DebuggerHelpers::read_memory_raw(core.gb, bank_of(bank, addr), addr);
    allow_memory_callbacks = true;
    return value;
}

DisassembledInstruction DebuggerBackend::do_disassemble(std::optional<uint16_t> bank, uint16_t addr) {
    const auto read_memory_func = [this, bank](uint16_t address) {
        return bank ? read_memory_raw(*bank, address) : read_memory(address);
    };

    uint32_t address_cursor = addr;
    uint8_t opcode = read_memory_func(address_cursor++);
    DisassembledInstruction instruction {opcode};

    // This works for CB and non-CB because all CB instructions have length 2
    uint8_t length = instruction_length(opcode);

    while (address_cursor < addr + length && address_cursor <= 0xFFFF) {
        instruction.push_back(read_memory_func(address_cursor++));
    }

    return instruction;
}

uint16_t DebuggerBackend::bank_of(std::optional<uint16_t> bank, uint16_t address) const {
    return bank ? *bank : bank_of(address);
}

uint16_t DebuggerBackend::bank_of(uint16_t addr) const {
    return DebuggerHelpers::get_bank_for_address(core.gb, addr);
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
    breakpoints.list.clear();
    breakpoints.by_address.clear();
    watchpoints.list.clear();
    watchpoints.by_address.clear();
    for (uint32_t i = 0; i < array_size(disassembled_instructions); i++) {
        disassembled_instructions[i].reset();
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

const std::vector<DisassembledInstructionRef>& DebuggerBackend::get_call_stack() const {
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