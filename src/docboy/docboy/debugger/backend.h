#ifndef BACKEND_H
#define BACKEND_H

#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <unordered_map>

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
    struct BankedAddressMapKey {
        Bank bank {};
        uint16_t address {};

        bool operator==(const BankedAddressMapKey& other) const;

        struct Hash {
            std::size_t operator()(const BankedAddressMapKey& key) const noexcept;
        };
    };

    explicit DebuggerBackend(Core& core);

    void attach_frontend(DebuggerFrontend& frontend);

    void load_symbols(const std::string& path);
    const DebugSymbol* get_symbol(Bank bank, uint16_t addr) const;
    const DebugSymbol* get_symbol(const std::string& name) const;
    const std::vector<DebugSymbol>& get_symbols() const;

    void notify_tick(uint64_t tick);
    void notify_memory_read(uint16_t address);
    void notify_memory_write(uint16_t address);

    void set_on_reset_callback(std::function<void()>&& callback);

    bool is_asking_to_quit() const;

    const CartridgeInfo& get_cartridge_info();

    uint32_t add_breakpoint(std::optional<Bank> bank, uint16_t addr);
    const Breakpoint* get_breakpoint(std::optional<Bank> bank, uint16_t addr) const;
    const std::list<Breakpoint>& get_breakpoints() const;

    uint32_t add_watchpoint(Watchpoint::Type, std::optional<Bank> bank, uint16_t from, uint16_t to, bool raw,
                            std::optional<Watchpoint::Condition>);
    const Watchpoint* get_watchpoint(std::optional<Bank> bank, uint16_t addr) const;
    const std::list<Watchpoint>& get_watchpoints() const;

    void remove_point(uint32_t id);
    void clear_points();

    std::optional<DisassembledInstructionRef> disassemble(std::optional<Bank> bank, uint16_t addr, bool cache = false);
    std::vector<DisassembledInstructionRef> disassemble_multi(std::optional<Bank> bank, uint16_t addr, uint16_t n = 1,
                                                              bool cache = false);
    std::vector<DisassembledInstructionRef> disassemble_range(std::optional<Bank> bank, uint16_t from, uint16_t to,
                                                              bool cache = false);
    const DisassembledInstruction* get_disassembled_instruction(Bank bank, uint16_t addr) const;
    std::vector<DisassembledInstructionRef> get_disassembled_instructions() const;

    const std::vector<DisassembledInstructionRef>& get_call_stack() const;

    uint32_t state_hash() const;

    uint8_t read_memory(uint16_t addr);
    uint8_t read_memory_raw(std::optional<Bank> bank, uint16_t addr);

    void proceed();
    void interrupt();

    void reset();

    bool save_state(const std::string& path) const;
    bool load_state(const std::string& path);

    const Core& get_core() const;

private:
    static constexpr uint16_t ROM_MAX_BANKS = 512; // 8 MB
#ifdef ENABLE_BOOTROM
    static constexpr uint16_t MAX_BANKS = ROM_MAX_BANKS + 1;
#else
    static constexpr uint16_t MAX_BANKS = ROM_MAX_BANKS;
#endif

    void clear();

    void pull_command(const ExecutionState& state);

    DisassembledInstruction do_disassemble(std::optional<Bank> bank, uint16_t addr);

    Bank bank_of(std::optional<Bank> bank, uint16_t address) const;
    Bank bank_of(uint16_t addr) const;

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

    struct {
        std::vector<DebugSymbol> list {};
        std::unordered_map<BankedAddressMapKey, const DebugSymbol*, BankedAddressMapKey::Hash> by_address;
        std::unordered_map<std::string, const DebugSymbol*> by_name;
    } symbols;

    struct {
        std::list<Breakpoint> list;
        std::unordered_map<BankedAddressMapKey, const Breakpoint*, BankedAddressMapKey::Hash> by_address {};
    } breakpoints;

    struct {
        std::list<Watchpoint> list;
        std::unordered_map<BankedAddressMapKey, const Watchpoint*, BankedAddressMapKey::Hash> by_address {};
    } watchpoints;

    std::optional<WatchpointHit> watchpoint_hit;

    uint32_t next_point_id {};

    std::unique_ptr<std::array<std::optional<DisassembledInstruction>, 0x10000>> disassembled_instructions[MAX_BANKS];

    bool allow_memory_callbacks {true};

    std::deque<std::vector<uint8_t>> history;

    std::optional<DisassembledInstructionRef> last_instruction;
    std::vector<DisassembledInstructionRef> call_stack;

    uint32_t memory_hash {};

    std::function<void()> on_reset_callback {};
};

#endif // BACKEND_H