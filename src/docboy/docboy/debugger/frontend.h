#ifndef FRONTEND_H
#define FRONTEND_H

#include <functional>
#include <optional>
#include <string>

#include "docboy/debugger/shared.h"

class DebuggerBackend;
class GameBoy;
class Core;

enum class MemoryOutputFormat : char {
    Binary = 'b',
    Decimal = 'd',
    Hexadecimal = 'x',
    Instruction = 'i',
    Hexdump = 'h'
};

class DebuggerFrontend {
public:
    explicit DebuggerFrontend(DebuggerBackend& backend);
    ~DebuggerFrontend();

    Command pull_command(const ExecutionState& state);
    void notify_tick(uint64_t tick);

    void set_on_pulling_command_callback(std::function<void()> callback);
    void set_on_command_pulled_callback(std::function<bool(const std::string& s)> callback);

private:
    struct DisplayEntry {
        uint32_t id {};

        struct Examine {
            MemoryOutputFormat format {};
            std::optional<uint8_t> format_arg {};
            uint16_t address {};
            size_t length {};
            bool raw {};
        };

        std::variant<Examine /*, ...*/> expression;
    };

    template <typename FrontendCommandType>
    std::optional<Command> handle_command(const FrontendCommandType& cmd);

    void print_ui(const ExecutionState& state) const;

    std::string dump_memory(uint16_t from, uint32_t n, MemoryOutputFormat fmt, std::optional<uint8_t> fmt_arg,
                            bool raw) const;
    std::string dump_display_entry(const DisplayEntry& d) const;

    DebuggerBackend& backend;
    const Core& core;
    const GameBoy& gb;

    std::string last_cmdline;
    std::vector<DisplayEntry> display_entries;
    uint32_t trace {};

    struct {
        uint16_t past {6};
        uint16_t next {10};
    } auto_disassemble_instructions {};

    bool reprint_ui {};

    std::function<void()> on_pulling_command_callback {};
    std::function<bool(const std::string& cmd)> on_command_pulled_callback {};
};

#endif // FRONTEND_H