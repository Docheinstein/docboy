#ifndef BACKEND_H
#define BACKEND_H

#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>

#include "docboy/debugger/shared.h"

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
    uint16_t stack_level {};
};
struct MicroNextCommandState {
    uint64_t counter {};
    uint16_t stack_level {};
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

    void attach_frontend(DebuggerFrontend& frontend);

    void load_symbols(const std::string& path);
    std::optional<DebugSymbol> get_symbol(uint16_t addr) const;
    const std::map<uint16_t, DebugSymbol>& get_symbols() const;

    void notify_tick(uint64_t tick);
    void notify_memory_read(uint16_t address);
    void notify_memory_write(uint16_t address);

    void set_on_reset_callback(std::function<void()>&& callback);

    bool is_asking_to_quit() const;

    const CartridgeInfo& get_cartridge_info();

    uint32_t add_breakpoint(uint16_t addr);
    std::optional<Breakpoint> get_breakpoint(uint16_t addr) const;
    const std::vector<Breakpoint>& get_breakpoints() const;

    uint32_t add_watchpoint(Watchpoint::Type, uint16_t from, uint16_t to, std::optional<Watchpoint::Condition>);
    std::optional<Watchpoint> get_watchpoint(uint16_t addr) const;
    const std::vector<Watchpoint>& get_watchpoints() const;
    bool has_watchpoint(uint16_t addr) const;

    void remove_point(uint32_t id);
    void clear_points();

    std::optional<DisassembledInstructionReference> disassemble(uint16_t addr, bool cache = false);
    std::vector<DisassembledInstructionReference> disassemble_multi(uint16_t addr, uint16_t n = 1, bool cache = false);
    std::vector<DisassembledInstructionReference> disassemble_range(uint16_t from, uint16_t to, bool cache = false);
    std::optional<DisassembledInstruction> get_disassembled_instruction(uint16_t addr) const;
    std::vector<DisassembledInstructionReference> get_disassembled_instructions() const;

    const std::vector<DisassembledInstructionReference>& get_call_stack() const;

    uint32_t state_hash() const;

    uint8_t read_memory(uint16_t addr);
    uint8_t read_memory_raw(uint16_t addr);

    void proceed();
    void interrupt();

    void reset();

    bool save_state(const std::string& path) const;
    bool load_state(const std::string& path);

    const Core& get_core() const;

private:
    void clear();

    void pull_command(const ExecutionState& state);

    std::optional<DisassembledInstruction> do_disassemble(uint16_t addr);

    template <typename CommandType>
    void init_command_state(const CommandType& cmd);

    template <typename CommandType, typename CommandStateType>
    void handle_command(const CommandType& cmd, CommandStateType& state);

    Core& core;
    DebuggerFrontend* frontend {};

    std::optional<Command> command;
    std::optional<CommandState> command_state;

    bool run {true};
    bool interrupted {};

    std::optional<CartridgeInfo> cartridge_info {};

    std::map<uint16_t, DebugSymbol> symbols;

    std::vector<Breakpoint> breakpoints;
    std::vector<Watchpoint> watchpoints;
    uint8_t watchpoints_at_address[UINT16_MAX + 1] {};
    std::optional<DisassembledInstruction> disassembled_instructions[0x10000] {};

    std::optional<WatchpointHit> watchpoint_hit;

    uint32_t next_point_id {};

    bool allow_memory_callbacks {true};

    std::deque<std::vector<uint8_t>> history;

    std::optional<DisassembledInstructionReference> last_instruction;
    std::vector<DisassembledInstructionReference> call_stack;

    uint32_t memory_hash {};

    std::function<void()> on_reset_callback {};
};

#endif // BACKEND_H