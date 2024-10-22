#include "docboy/debugger/frontend.h"

#include <csignal>
#include <iostream>
#include <list>
#include <optional>
#include <regex>
#include <utility>

#include "docboy/cartridge/mbc1/mbc1.h"
#include "docboy/cartridge/mbc1/mbc1m.h"
#include "docboy/cartridge/mbc2/mbc2.h"
#include "docboy/cartridge/mbc3/mbc3.h"
#include "docboy/cartridge/mbc5/mbc5.h"
#include "docboy/cartridge/nombc/nombc.h"
#include "docboy/core/core.h"
#include "docboy/debugger/backend.h"
#include "docboy/debugger/helpers.h"
#include "docboy/debugger/mnemonics.h"
#include "docboy/debugger/rgbtoansi.h"
#include "docboy/debugger/shared.h"

#include "utils/formatters.h"
#include "utils/hexdump.h"
#include "utils/memory.h"
#include "utils/strings.h"

#include "tui/block.h"
#include "tui/decorators.h"
#include "tui/divider.h"
#include "tui/factory.h"
#include "tui/hlayout.h"
#include "tui/presenter.h"
#include "tui/vlayout.h"

namespace {
struct FrontendBreakpointCommand {
    uint16_t address {};
};

struct FrontendWatchpointCommand {
    Watchpoint::Type type {};
    uint16_t length {};
    struct {
        uint16_t from {};
        uint16_t to {};
    } address;
    struct {
        bool enabled {};
        Watchpoint::Condition condition {};
    } condition;
};

struct FrontendDeleteCommand {
    std::optional<uint16_t> num {};
};

struct FrontendAutoDisassembleCommand {
    uint16_t past {};
    uint16_t next {};
};

struct FrontendExamineCommand {
    MemoryOutputFormat format {};
    std::optional<uint8_t> format_arg {};
    uint32_t length {};
    uint16_t address {};
    bool raw {};
};

struct FrontendSearchBytesCommand {
    std::vector<uint8_t> bytes;
};

struct FrontendSearchInstructionsCommand {
    std::vector<uint8_t> instruction;
};

struct FrontendDisplayCommand {
    MemoryOutputFormat format {};
    std::optional<uint8_t> format_arg {};
    uint32_t length {};
    uint16_t address {};
    bool raw {};
};

struct FrontendUndisplayCommand {};

struct FrontendTickCommand {
    uint64_t count {};
};

struct FrontendDotCommand {
    uint64_t count {};
};

struct FrontendStepCommand {
    uint64_t count {};
};

struct FrontendMicroStepCommand {
    uint64_t count {};
};

struct FrontendNextCommand {
    uint64_t count {};
};

struct FrontendMicroNextCommand {
    uint64_t count {};
};

struct FrontendScanlineCommand {
    uint64_t count {};
};

struct FrontendFrameCommand {
    uint64_t count {};
};

struct FrontendFrameBackCommand {
    uint64_t count {};
};

struct FrontendContinueCommand {
    std::optional<uint16_t> address {};
};

struct FrontendTraceCommand {
    std::optional<uint32_t> level {};
};

struct FrontendChecksumCommand {};

struct FrontendDumpDisassembleCommand {};

struct FrontendHelpCommand {};
struct FrontendQuitCommand {};

using FrontendCommand =
    std::variant<FrontendBreakpointCommand, FrontendWatchpointCommand, FrontendDeleteCommand,
                 FrontendAutoDisassembleCommand, FrontendExamineCommand, FrontendSearchBytesCommand,
                 FrontendSearchInstructionsCommand, FrontendDisplayCommand, FrontendUndisplayCommand,
                 FrontendTickCommand, FrontendDotCommand, FrontendStepCommand, FrontendMicroStepCommand,
                 FrontendNextCommand, FrontendMicroNextCommand, FrontendFrameCommand, FrontendScanlineCommand,
                 FrontendFrameBackCommand, FrontendContinueCommand, FrontendTraceCommand, FrontendChecksumCommand,
                 FrontendDumpDisassembleCommand, FrontendHelpCommand, FrontendQuitCommand>;

struct FrontendCommandInfo {
    std::regex regex;
    std::string format;
    std::string help;
    std::optional<FrontendCommand> (*parser)(const std::vector<std::string>& groups);
};

enum TraceFlag : uint32_t {
    TraceFlagInstruction = 1 << 0,
    TraceFlagRegisters = 1 << 1,
    TraceFlagInterrupts = 1 << 2,
    TraceFlagTimers = 1 << 3,
    TraceFlagDma = 1 << 4,
    TraceFlagStack = 1 << 5,
    TraceFlagMemory = 1 << 6,
    TraceFlagPpu = 1 << 7,
    TraceFlagHash = 1 << 8,
    TraceFlagMCycle = 1 << 9,
    TraceFlagTCycle = 1 << 10,
    MaxTraceFlag = (TraceFlagTCycle << 1) - 1,
};

template <typename T>
T parse_hex(const std::string& s, bool* ok) {
    const char* cs = s.c_str();
    char* end_ptr;
    T result = std::strtol(cs, &end_ptr, 16);
    if (end_ptr == cs || *end_ptr != '\0') {
        if (ok) {
            *ok = false;
        }
        return 0;
    }

    return result;
}

std::vector<uint8_t> parse_hex_str(const std::string& s, bool* ok) {
    const std::string ss = (s.size() % 2 == 0) ? s : ("0" + s);
    std::vector<uint8_t> result {};
    for (uint32_t i = 0; i < ss.size() && (!ok || (*ok)); i += 2) {
        result.push_back(parse_hex<uint8_t>(ss.substr(i, 2), ok));
    }
    return result;
}

uint16_t address_str_to_addr(const std::string& s, bool* ok) {
    if (std::optional<uint16_t> addr = mnemonic_to_address(s)) {
        return *addr;
    }
    return parse_hex<uint16_t>(s, ok);
}

FrontendCommandInfo FRONTEND_COMMANDS[] {
    {std::regex(R"(b\s+(\w+))"), "b <addr>", "Set breakpoint at <addr>",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendBreakpointCommand cmd {};
         bool ok {true};
         cmd.address = address_str_to_addr(groups[0], &ok);
         return ok ? std::optional(cmd) : std::nullopt;
     }},
    {std::regex(R"(w(?:/([ra]))?\s+(\w+),(\w+)\s*(.*)?)"), "w[/r|a] <start>,<end> [<cond>]",
     "Set watchpoint from <start> to <end>",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendWatchpointCommand cmd {};
         bool ok {true};
         const std::string& access = groups[0];
         const std::string& from = groups[1];
         const std::string& to = groups[2];
         const std::string& condition = groups[3];

         if (access == "r") {
             cmd.type = Watchpoint::Type::Read;
         } else if (access == "a") {
             cmd.type = Watchpoint::Type::ReadWrite;
         } else {
             cmd.type = Watchpoint::Type::Change;
         }

         cmd.address.from = address_str_to_addr(from, &ok);
         cmd.address.to = address_str_to_addr(to, &ok);

         std::vector<std::string> tokens;
         split(condition, std::back_inserter(tokens));
         if (tokens.size() >= 2) {
             const auto& operation = tokens[0];
             const auto& operand = tokens[1];
             if (operation == "==") {
                 cmd.condition.enabled = true;
                 cmd.condition.condition.operation = Watchpoint::Condition::Operator::Equal;
                 cmd.condition.condition.operand = address_str_to_addr(operand, &ok);
             }
         }

         return ok ? std::optional(cmd) : std::nullopt;
     }},
    {std::regex(R"(w(?:/([ra]))?\s+(\w+)\s*(.*)?)"), "w[/r|a] <addr> [<cond>]", "Set watchpoint at <addr>",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendWatchpointCommand cmd {};
         bool ok {true};
         const std::string& access = groups[0];
         const std::string& from = groups[1];
         const std::string& condition = groups[2];

         if (access == "r") {
             cmd.type = Watchpoint::Type::Read;
         } else if (access == "a") {
             cmd.type = Watchpoint::Type::ReadWrite;
         } else {
             cmd.type = Watchpoint::Type::Change;
         }

         cmd.address.from = cmd.address.to = address_str_to_addr(from, &ok);

         std::vector<std::string> tokens;
         split(condition, std::back_inserter(tokens));
         if (tokens.size() >= 2) {
             const auto& operation = tokens[0];
             const auto& operand = tokens[1];
             if (operation == "==") {
                 cmd.condition.enabled = true;
                 cmd.condition.condition.operation = Watchpoint::Condition::Operator::Equal;
                 cmd.condition.condition.operand = address_str_to_addr(operand, &ok);
             }
         }

         return ok ? std::optional(cmd) : std::nullopt;
     }},
    {std::regex(R"(del\s*(\d+)?)"), "del <num>", "Delete breakpoint or watchpoint <num>",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendDeleteCommand cmd {};
         const std::string& num = groups[0];
         if (!num.empty()) {
             cmd.num = std::stoi(num);
         }
         return cmd;
     }},
    {std::regex(R"(ad\s*(\d+)\s+(\d+))"), "ad <past> <next>",
     "Automatically disassemble past <past> and next <next> instructions",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendAutoDisassembleCommand cmd {};
         const std::string& past = groups[0];
         const std::string& next = groups[1];
         cmd.past = std::stoi(past);
         cmd.next = std::stoi(next);
         return cmd;
     }},
    {std::regex(R"(ad\s*(\d+)?)"), "ad <num>", "Automatically disassemble next <num> instructions (default = 10)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendAutoDisassembleCommand cmd {};
         const std::string& n = groups[0];
         cmd.next = !n.empty() ? std::stoi(n) : 10;
         return cmd;
     }},
    {std::regex(R"(x(x)?(?:/(\d+)?(?:([xhbdi])(\d+)?)?)?\s+(\w+))"), "x[x][/<length><format>] <addr>",
     "Display memory at <addr> (x: raw) (<format>: x, h[<cols>], b, d, i)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendExamineCommand cmd {};
         bool ok {true};
         const std::string& raw = groups[0];
         const std::string& length = groups[1];
         const std::string& format = groups[2];
         const std::string& format_arg = groups[3];
         const std::string& address = groups[4];
         cmd.raw = !raw.empty();
         cmd.length = length.empty() ? 1 : std::stoi(length);
         cmd.format = MemoryOutputFormat(format.empty() ? 'x' : format[0]);
         if (!format_arg.empty()) {
             cmd.format_arg = stoi(format_arg);
         }
         cmd.address = address_str_to_addr(address, &ok);
         return ok ? std::optional(cmd) : std::nullopt;
     }},
    {std::regex(R"(/b\s+(\w+))"), "/b <bytes>", "Search for <bytes>",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendSearchBytesCommand cmd {};
         bool ok {true};
         const std::string& bytes = groups[0];
         cmd.bytes = parse_hex_str(bytes, &ok);
         return ok ? std::optional(cmd) : std::nullopt;
     }},
    {std::regex(R"(/i\s+(\w+))"), "/i <bytes>", "Search for instructions matching <bytes>",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendSearchInstructionsCommand cmd {};
         bool ok {true};
         const std::string& bytes = groups[0];
         cmd.instruction = parse_hex_str(bytes, &ok);
         return ok ? std::optional(cmd) : std::nullopt;
     }},
    {std::regex(R"(display(x)?(?:/(\d+)?(?:([xhbdi])(\d+)?)?)?\s+(\w+))"), "display[x][/<length><format>] <addr>",
     "Automatically display memory at <addr> (x: raw) (<format>: x, h[<cols>], b, d, i)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendDisplayCommand cmd {};
         bool ok {true};
         const std::string& raw = groups[0];
         const std::string& length = groups[1];
         const std::string& format = groups[2];
         const std::string& format_arg = groups[3];
         const std::string& address = groups[4];
         cmd.raw = !raw.empty();
         cmd.length = length.empty() ? 1 : std::stoi(length);
         cmd.format = MemoryOutputFormat(format.empty() ? 'x' : format[0]);
         if (!format_arg.empty()) {
             cmd.format_arg = stoi(format_arg);
         }
         cmd.address = address_str_to_addr(address, &ok);
         return ok ? std::optional(cmd) : std::nullopt;
     }},
    {std::regex(R"(undisplay)"), "undisplay", "Undisplay expressions set with display",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendUndisplayCommand cmd {};
         return cmd;
     }},
    {std::regex(R"(t\s*(\d+)?)"), "t [<count>]", "Continue running for <count> clock ticks (default = 1)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendTickCommand {n};
     }},
    {std::regex(R"(\.\s*(\d+)?)"), ". [<count>]", "Continue running for <count> PPU dots (default = 1)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendDotCommand {n};
     }},
    {std::regex(R"(s\s*(\d+)?)"), "s [<count>]", "Continue running for <count> instructions (default = 1)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendStepCommand {n};
     }},
    {std::regex(R"(si\s*(\d+)?)"), "si [<count>]", "Continue running for <count> micro-operations (default = 1)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendMicroStepCommand {n};
     }},
    {std::regex(R"(n\s*(\d+)?)"), "n [<count>]",
     "Continue running for <count> instructions at the same stack level (default = 1)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendNextCommand {n};
     }},
    {std::regex(R"(ni\s*(\d+)?)"), "ni [<count>]",
     "Continue running for <count> micro-operations at the same stack level (default = 1)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendMicroNextCommand {n};
     }},
    {std::regex(R"(f\s*(\d+)?)"), "f [<count>]", "Continue running for <count> frames (default = 1)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendFrameCommand {n};
     }},
    {std::regex(R"(fb\s*(\d+)?)"), "fb [<count>]",
     "Step back by <count> frames (default = 1, max = " + std::to_string(600) + ")",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendFrameBackCommand {n};
     }},
    {std::regex(R"(l\s*(\d+)?)"), "l [<count>]", "Continue running for <count> lines (default = 1)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return FrontendScanlineCommand {n};
     }},
    {std::regex(R"(c\s*(\w+)?)"), "c [<address>]", "Continue (optionally stop at <address>)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendContinueCommand cmd {};
         const std::string& addr = groups[0];
         if (!addr.empty()) {
             bool ok {true};
             cmd.address = address_str_to_addr(groups[0], &ok);
             if (!ok) {
                 return std::nullopt;
             }
         }
         return cmd;
     }},
    {std::regex(R"(trace\s*(\d+)?)"), "trace [<level>]", "Set the trace level or toggle it (output on stderr)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& level = groups[0];
         FrontendTraceCommand cmd;
         if (!level.empty()) {
             cmd.level = std::stoi(level);
         }
         return cmd;
     }},
    {std::regex(R"(dump)"), "dump", "Dump the disassemble (output on stderr)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         return FrontendDumpDisassembleCommand {};
     }},
    {std::regex(R"(h(?:elp)?)"), "h", "Display help",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         return FrontendHelpCommand {};
     }},
    {std::regex(R"(q)"), "q", "Quit",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         return FrontendQuitCommand {};
     }},
};

std::optional<FrontendCommand> parse_cmdline(const std::string& s) {
    for (const auto& command : FRONTEND_COMMANDS) {
        std::smatch match;
        if (std::regex_match(s, match, command.regex)) {
            std::vector<std::string> groups;
            std::copy(match.begin() + 1, match.end(), std::back_inserter(groups));
            return command.parser(groups);
        }
    }
    return std::nullopt;
}

volatile sig_atomic_t sigint_trigger = 0;

void sigint_handler(int signum) {
#ifndef POSIX
    signal(SIGINT, sigint_handler);
#endif

    sigint_trigger = 1;
}

#ifdef POSIX
void attach_sigint_handler() {
    struct sigaction sa {};
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
}

void detach_sigint_handler() {
    sigaction(SIGINT, nullptr, nullptr);
}
#else
void attach_sigint_handler() {
    signal(SIGINT, sigint_handler);
}

void detach_sigint_handler() {
    signal(SIGINT, SIG_DFL);
}
#endif

std::string to_string(const DisassembledInstructionReference& instr) {
    std::stringstream ss;
    ss << hex(instr.address) << "  :  " << std::left << std::setw(9) << hex(instr.instruction) << "   " << std::left
       << std::setw(23) << instruction_mnemonic(instr.instruction, instr.address);
    return ss.str();
}

#ifdef TERM_SUPPORTS_UNICODE
constexpr const char* PIPE = "|";
constexpr const char* DASH = "—";
constexpr const char* DOT = "•";
#else
constexpr const char* PIPE = "|";
constexpr const char* DASH = "-";
constexpr const char* DOT = "*";
#endif
} // namespace

DebuggerFrontend::DebuggerFrontend(DebuggerBackend& backend) :
    backend {backend},
    core {backend.get_core()},
    gb {core.gb} {
    attach_sigint_handler();
}

DebuggerFrontend::~DebuggerFrontend() {
    detach_sigint_handler();
}

// ============================= COMMANDS ======================================

// Breakpoint
template <>
std::optional<Command>
DebuggerFrontend::handle_command<FrontendBreakpointCommand>(const FrontendBreakpointCommand& cmd) {
    uint32_t id = backend.add_breakpoint(cmd.address);
    std::cout << "Breakpoint [" << id << "] at " << hex(cmd.address) << std::endl;
    reprint_ui = true;
    return std::nullopt;
}

// Watchpoint
template <>
std::optional<Command>
DebuggerFrontend::handle_command<FrontendWatchpointCommand>(const FrontendWatchpointCommand& cmd) {
    std::optional<Watchpoint::Condition> cond;
    if (cmd.condition.enabled) {
        cond = cmd.condition.condition;
    }
    uint32_t id = backend.add_watchpoint(cmd.type, cmd.address.from, cmd.address.to, cond);
    if (cmd.address.from == cmd.address.to) {
        std::cout << "Watchpoint [" << id << "] at " << hex(cmd.address.from) << std::endl;
    } else {
        std::cout << "Watchpoint [" << id << "] at " << hex(cmd.address.from) << " - " << hex(cmd.address.to)
                  << std::endl;
    }
    reprint_ui = true;
    return std::nullopt;
}

// Delete
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendDeleteCommand>(const FrontendDeleteCommand& cmd) {
    if (cmd.num) {
        backend.remove_point(*cmd.num);
    } else {
        backend.clear_points();
    }
    reprint_ui = true;
    return std::nullopt;
}

// AutoDisassemble
template <>
std::optional<Command>
DebuggerFrontend::handle_command<FrontendAutoDisassembleCommand>(const FrontendAutoDisassembleCommand& cmd) {
    auto_disassemble_instructions.past = cmd.past;
    auto_disassemble_instructions.next = cmd.next;
    reprint_ui = true;
    return std::nullopt;
}

// Examine
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendExamineCommand>(const FrontendExamineCommand& cmd) {
    std::cout << dump_memory(cmd.address, cmd.length, cmd.format, cmd.format_arg, cmd.raw) << std::endl;
    return std::nullopt;
}

// SearchBytes
template <>
std::optional<Command>
DebuggerFrontend::handle_command<FrontendSearchBytesCommand>(const FrontendSearchBytesCommand& cmd) {
    // TODO: handle bytes in different MBC banks?

    // read all memory
    std::vector<uint8_t> mem {};
    mem.resize(0x10000);
    for (uint32_t addr = 0; addr <= 0xFFFF; addr++) {
        mem[addr] = backend.read_memory(addr);
    }

    // search for bytes in memory
    std::vector<uint32_t> occurrences = mem_find_all(mem.data(), mem.size(), cmd.bytes.data(), cmd.bytes.size());
    for (const auto& addr : occurrences) {
        std::cout << hex((uint16_t)addr) << std::endl;
    }

    return std::nullopt;
}

// SearchInstructions
template <>
std::optional<Command>
DebuggerFrontend::handle_command<FrontendSearchInstructionsCommand>(const FrontendSearchInstructionsCommand& cmd) {
    // find instruction among known disassembled instructions
    for (const auto& [addr, instr] : backend.get_disassembled_instructions()) {
        if (mem_find_first(instr.data(), instr.size(), cmd.instruction.data(), cmd.instruction.size())) {
            std::cout << hex(addr) << "    " << instruction_mnemonic(instr, addr) << std::endl;
        }
    }
    return std::nullopt;
}

// Display
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendDisplayCommand>(const FrontendDisplayCommand& cmd) {
    DisplayEntry d = {static_cast<uint32_t>(display_entries.size()),
                      DisplayEntry::Examine {cmd.format, cmd.format_arg, cmd.address, cmd.length, cmd.raw}};
    display_entries.push_back(d);
    std::cout << dump_display_entry(d);
    reprint_ui = true;
    return std::nullopt;
}

// Undisplay
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendUndisplayCommand>(const FrontendUndisplayCommand& cmd) {
    display_entries.clear();
    reprint_ui = true;
    return std::nullopt;
}

// Tick
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendTickCommand>(const FrontendTickCommand& cmd) {
    return TickCommand {cmd.count};
}

// Dot
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendDotCommand>(const FrontendDotCommand& cmd) {
    return DotCommand {cmd.count};
}

// Step
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendStepCommand>(const FrontendStepCommand& cmd) {
    return StepCommand {cmd.count};
}

// MicroStep
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendMicroStepCommand>(const FrontendMicroStepCommand& cmd) {
    return MicroStepCommand {cmd.count};
}

// Next
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendNextCommand>(const FrontendNextCommand& cmd) {
    return NextCommand {cmd.count};
}

// MicroNext
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendMicroNextCommand>(const FrontendMicroNextCommand& cmd) {
    return MicroNextCommand {cmd.count};
}

// Frame
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendFrameCommand>(const FrontendFrameCommand& cmd) {
    return FrameCommand {cmd.count};
}

// FrameBack
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendFrameBackCommand>(const FrontendFrameBackCommand& cmd) {
    return FrameBackCommand {cmd.count};
}

// Scanline
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendScanlineCommand>(const FrontendScanlineCommand& cmd) {
    return ScanlineCommand {cmd.count};
}

// Continue
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendContinueCommand>(const FrontendContinueCommand& cmd) {
    if (cmd.address) {
        temporary_breakpoint = backend.add_breakpoint(*cmd.address);
    }

    return ContinueCommand();
}

// Trace
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendTraceCommand>(const FrontendTraceCommand& cmd) {
    if (cmd.level) {
        trace = *cmd.level;
    } else {
        trace = trace ? 0 : MaxTraceFlag;
    }

    std::cout << "Trace          :     | " << trace << std::endl;
    std::cout << "> Instructions : " << (static_cast<bool>(trace & TraceFlagInstruction) ? "ON " : "OFF") << " | "
              << TraceFlagInstruction << std::endl;
    std::cout << "> Registers    : " << (static_cast<bool>(trace & TraceFlagRegisters) ? "ON " : "OFF") << " | "
              << TraceFlagRegisters << std::endl;
    std::cout << "> Interrupts   : " << (static_cast<bool>(trace & TraceFlagInterrupts) ? "ON " : "OFF") << " | "
              << TraceFlagInterrupts << std::endl;
    std::cout << "> Timers       : " << (static_cast<bool>(trace & TraceFlagTimers) ? "ON " : "OFF") << " | "
              << TraceFlagTimers << std::endl;
    std::cout << "> DMA          : " << (static_cast<bool>(trace & TraceFlagDma) ? "ON " : "OFF") << " | "
              << TraceFlagDma << std::endl;
    std::cout << "> Stack        : " << (static_cast<bool>(trace & TraceFlagStack) ? "ON " : "OFF") << " | "
              << TraceFlagStack << std::endl;
    std::cout << "> Memory       : " << (static_cast<bool>(trace & TraceFlagMemory) ? "ON " : "OFF") << " | "
              << TraceFlagMemory << std::endl;
    std::cout << "> PPU          : " << (static_cast<bool>(trace & TraceFlagPpu) ? "ON " : "OFF") << " | "
              << TraceFlagPpu << std::endl;
    std::cout << "> Hash         : " << (static_cast<bool>(trace & TraceFlagHash) ? "ON " : "OFF") << " | "
              << TraceFlagHash << std::endl;
    std::cout << "> M-Cycles     : " << (static_cast<bool>(trace & TraceFlagMCycle) ? "ON " : "OFF") << " | "
              << TraceFlagMCycle << std::endl;
    std::cout << "> T-Cycles     : " << (static_cast<bool>(trace & TraceFlagTCycle) ? "ON " : "OFF") << " | "
              << TraceFlagTCycle << std::endl;

    return std::nullopt;
}

// DumpDisassemble
template <>
std::optional<Command>
DebuggerFrontend::handle_command<FrontendDumpDisassembleCommand>(const FrontendDumpDisassembleCommand& cmd) {
    std::vector<DisassembledInstructionReference> disassembled = backend.get_disassembled_instructions();
    for (uint32_t i = 0; i < disassembled.size(); i++) {
        const auto& instr = disassembled[i];
        std::cerr << to_string(instr) << std::endl;
        if (i < disassembled.size() - 1 && instr.address + instr.instruction.size() != disassembled[i + 1].address) {
            std::cerr << std::endl; // next instruction is not adjacent to this one
        }
    }
    std::cout << "Dumped " << disassembled.size() << " instructions" << std::endl;

    return std::nullopt;
}

// Help
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendHelpCommand>(const FrontendHelpCommand& cmd) {
    auto it = std::max_element(std::begin(FRONTEND_COMMANDS), std::end(FRONTEND_COMMANDS),
                               [](const FrontendCommandInfo& i1, const FrontendCommandInfo& i2) {
                                   return i1.format.length() < i2.format.length();
                               });
    const int longest_command_format = static_cast<int>(it->format.length());
    for (const auto& info : FRONTEND_COMMANDS) {
        std::cout << std::left << std::setw(longest_command_format) << info.format << " : " << info.help << std::endl;
    }
    return std::nullopt;
}

// Quit
template <>
std::optional<Command> DebuggerFrontend::handle_command<FrontendQuitCommand>(const FrontendQuitCommand& cmd) {
    return AbortCommand();
}

Command DebuggerFrontend::pull_command(const ExecutionState& state) {
    using namespace Tui;

    std::optional<Command> cmd_to_send {};
    std::string cmdline;

    reprint_ui = true;

    // Eventually remove the temporary breakpoint
    if (temporary_breakpoint) {
        backend.remove_point(*temporary_breakpoint);
        temporary_breakpoint = std::nullopt;
    }

    // Pull command loop
    do {
        if (reprint_ui) {
            reprint_ui = false;
            print_ui(state);
        }

        // Prompt
        std::cout << bold(green(">>")).str() << std::endl;

        // Eventually notify observers of this frontend that a command
        // is about to be pulled
        if (on_pulling_command_callback) {
            on_pulling_command_callback();
        }

        sigint_trigger = false;
        getline(std::cin, cmdline);

        if (sigint_trigger) {
            // CTRL+C
            std::cin.clear();
            std::cout << std::endl;
            continue;
        }

        if (std::cin.fail() || std::cin.eof()) {
            // CTRL+D
            return AbortCommand();
        }

        if (cmdline.empty()) {
            cmdline = last_cmdline;
        } else {
            last_cmdline = cmdline;
        }

        // Eventually notify observers of this frontend of the pulled command
        if (on_command_pulled_callback) {
            if (on_command_pulled_callback(cmdline)) {
                // handled by the observer
                continue;
            }
        }

        // Parse command
        std::optional<FrontendCommand> parse_result = parse_cmdline(cmdline);
        if (!parse_result) {
            std::cout << "Invalid command" << std::endl;
            continue;
        }

        const FrontendCommand& cmd = *parse_result;

        if (std::holds_alternative<FrontendBreakpointCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendBreakpointCommand>(cmd));
        } else if (std::holds_alternative<FrontendWatchpointCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendWatchpointCommand>(cmd));
        } else if (std::holds_alternative<FrontendDeleteCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendDeleteCommand>(cmd));
        } else if (std::holds_alternative<FrontendAutoDisassembleCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendAutoDisassembleCommand>(cmd));
        } else if (std::holds_alternative<FrontendExamineCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendExamineCommand>(cmd));
        } else if (std::holds_alternative<FrontendSearchBytesCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendSearchBytesCommand>(cmd));
        } else if (std::holds_alternative<FrontendSearchInstructionsCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendSearchInstructionsCommand>(cmd));
        } else if (std::holds_alternative<FrontendDisplayCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendDisplayCommand>(cmd));
        } else if (std::holds_alternative<FrontendUndisplayCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendUndisplayCommand>(cmd));
        } else if (std::holds_alternative<FrontendTickCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendTickCommand>(cmd));
        } else if (std::holds_alternative<FrontendDotCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendDotCommand>(cmd));
        } else if (std::holds_alternative<FrontendStepCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendStepCommand>(cmd));
        } else if (std::holds_alternative<FrontendMicroStepCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendMicroStepCommand>(cmd));
        } else if (std::holds_alternative<FrontendNextCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendNextCommand>(cmd));
        } else if (std::holds_alternative<FrontendMicroNextCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendMicroNextCommand>(cmd));
        } else if (std::holds_alternative<FrontendFrameCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendFrameCommand>(cmd));
        } else if (std::holds_alternative<FrontendFrameBackCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendFrameBackCommand>(cmd));
        } else if (std::holds_alternative<FrontendScanlineCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendScanlineCommand>(cmd));
        } else if (std::holds_alternative<FrontendContinueCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendContinueCommand>(cmd));
        } else if (std::holds_alternative<FrontendTraceCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendTraceCommand>(cmd));
        } else if (std::holds_alternative<FrontendDumpDisassembleCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendDumpDisassembleCommand>(cmd));
        } else if (std::holds_alternative<FrontendHelpCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendHelpCommand>(cmd));
        } else if (std::holds_alternative<FrontendQuitCommand>(cmd)) {
            cmd_to_send = handle_command(std::get<FrontendQuitCommand>(cmd));
        }
    } while (!cmd_to_send);

    return *cmd_to_send;
}

void DebuggerFrontend::notify_tick(uint64_t tick) {
    if (trace) {
        const Cpu& cpu = gb.cpu;

        const bool m_trace = (trace & TraceFlagMCycle) || (trace & TraceFlagTCycle);
        const bool t_trace = (trace & TraceFlagTCycle);
        const bool is_m_cycle = tick % 4 == 0;
        const bool is_new_instruction = cpu.instruction.microop.counter == 0;

        if (!(t_trace || (is_m_cycle && m_trace) || (is_m_cycle && is_new_instruction))) {
            // do not trace
            return;
        }

        std::cerr << std::left << std::setw(12) << "[" + std::to_string(tick) + "] ";

        if (trace & TraceFlagInstruction) {
            std::string instr_str;
            if (cpu.halted) {
                instr_str = "<HALTED>";
            } else {
                if (DebuggerHelpers::is_in_isr(cpu)) {
                    instr_str = "isr " + hex(cpu.instruction.address);
                } else {
                    const auto instr = backend.disassemble(cpu.instruction.address);
                    if (instr) {
                        instr_str = instruction_mnemonic(instr->instruction, cpu.instruction.address);
                    } else {
                        instr_str = "unknown";
                    }
                }
            }

            std::cerr << std::left << std::setw(28) << instr_str << "  ";
        }

        if (trace & TraceFlagRegisters) {
            std::cerr << "AF:" << hex(cpu.af) << " BC:" << hex(cpu.bc) << " DE:" << hex(cpu.de) << " HL:" << hex(cpu.hl)
                      << " SP:" << hex(cpu.sp) << " PC:" << hex((uint16_t)(cpu.pc)) << "  ";
        }

        if (trace & TraceFlagInterrupts) {
            const Interrupts& interrupts = gb.interrupts;
            std::cerr << "IME:"
                      << (cpu.ime == Cpu::ImeState::Enabled
                              ? "1"
                              : ((cpu.ime == Cpu::ImeState::Pending || cpu.ime == Cpu::ImeState::Requested) ? "!"
                                                                                                            : "0"))
                      << " IE:" << hex((uint8_t)interrupts.IE) << " IF:" << hex((uint8_t)interrupts.IF) << "  ";
        }

        if (trace & TraceFlagDma) {
            std::cerr << "DMA:" << std::setw(3) << (gb.dma.transferring ? std::to_string(gb.dma.cursor) : "OFF")
                      << "  ";
        }

        if (trace & TraceFlagTimers) {
            std::cerr << "DIV16:" << hex(gb.timers.div16)
                      << " DIV:" << hex(backend.read_memory(Specs::Registers::Timers::DIV))
                      << " TIMA:" << hex(backend.read_memory(Specs::Registers::Timers::TIMA))
                      << " TMA:" << hex(backend.read_memory(Specs::Registers::Timers::TMA))
                      << " TAC:" << hex(backend.read_memory(Specs::Registers::Timers::TAC)) << "  ";
        }

        if (trace & TraceFlagStack) {
            std::cerr << "[SP+1]:" << hex(backend.read_memory(cpu.sp + 1)) << " "
                      << "[SP]:" << hex(backend.read_memory(cpu.sp)) << " "
                      << "[SP-1]:" << hex(backend.read_memory(cpu.sp - 1)) << "  ";
        }

        if (trace & TraceFlagMemory) {
            std::cerr << "[BC]:" << hex(backend.read_memory(cpu.bc)) << " "
                      << "[DE]:" << hex(backend.read_memory(cpu.de)) << " "
                      << "[HL]:" << hex(backend.read_memory(cpu.hl)) << "  ";
        }

        if (trace & TraceFlagPpu) {
            const Ppu& ppu = gb.ppu;
            std::cerr << "Mode:" << +gb.ppu.stat.mode << " Dots:" << std::setw(12) << ppu.dots << "  ";
        }

        if (trace & TraceFlagHash) {
            std::cerr << "Hash:" << hex(static_cast<uint16_t>(backend.state_hash())) << "  ";
        }

        std::cerr << std::endl;
    }

    if (sigint_trigger) {
        sigint_trigger = 0;
        backend.interrupt();
    }
}

void DebuggerFrontend::set_on_pulling_command_callback(std::function<void()> callback) {
    on_pulling_command_callback = std::move(callback);
}

void DebuggerFrontend::set_on_command_pulled_callback(std::function<bool(const std::string&)> callback) {
    on_command_pulled_callback = std::move(callback);
}

// ================================= UI ========================================

void DebuggerFrontend::print_ui(const ExecutionState& execution_state) const {
    using namespace Tui;

    const auto title = [](const Text& text, uint16_t width, const std::string& sep = DASH) {
        ASSERT(width >= text.size());
        Text t {text.size() > 0 ? (" " + text + " ") : text};

        Token sep_token {sep, true};

        const uint16_t w1 = (width - t.size()) / 2;
        Text pre;
        for (uint16_t i = 0; i < w1; i++) {
            pre += sep_token;
        }

        const uint16_t w2 = width - t.size() - w1;
        Text post;
        for (uint16_t i = 0; i < w2; i++) {
            post += sep_token;
        }

        return darkgray(std::move(pre)) + t + darkgray(std::move(post));
    };

    const auto header = [title](const char* c, uint16_t width) {
        return title(bold(lightcyan(c)), width);
    };

    const auto subheader = [title](const char* c, uint16_t width) {
        return title(cyan(c), width);
    };

    const auto subheader2 = [title](const char* c, uint16_t width) {
        return title(lightmagenta(c), width);
    };

    const auto hr = [title](uint16_t width) {
        return title("", width);
    };

    const auto colorbool = [](bool b) -> Text {
        return b ? b : gray(b);
    };

    const auto make_horizontal_line_divider = []() {
        return make_divider(Text {} + "  " + Token {PIPE, 1} + "  ");
    };

    const auto make_vertical_line_divider = []() {
        return make_divider(Text {Token {DASH, 1}});
    };

    const auto make_space_divider = []() {
        return make_divider(" ");
    };

    // Gameboy
    const auto make_gameboy_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("GAMEBOY", width) << endl;

        b << yellow("T-Cycle") << "  :  " << core.ticks << endl;
        b << yellow("M-Cycle") << "  :  " << core.ticks / 4 << endl;

        b << yellow("Phase") << "    :  ";
        for (uint8_t t = 0; t < 4; t++) {
            Text phase {"T" + std::to_string(t)};
            b << (core.ticks % 4 == t ? phase : darkgray(std::move(phase)));
            if (t < 3) {
                b << " ";
            }
        }
        b << endl;

        b << yellow("Hash") << "     :  " << hex(static_cast<uint16_t>(backend.state_hash())) << endl;

        return b;
    };

    // CPU
    const auto make_cpu_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("CPU", width) << endl;

        // State
        b << yellow("Cycle") << "   :  " << gb.cpu.cycles << endl;
        b << yellow("Halted") << "  :  " << gb.cpu.halted << endl;

        // Registers
        const auto reg = [](uint16_t value) {
            return bin((uint8_t)(value >> 8)) + " " + bin((uint8_t)(value & 0xFF)) + " (" + hex((uint8_t)(value >> 8)) +
                   " " + hex((uint8_t)(value & 0xFF)) + ")";
        };
        b << subheader("registers", width) << endl;
        b << red("AF") << "      :  " << reg(gb.cpu.af) << endl;
        b << red("BC") << "      :  " << reg(gb.cpu.bc) << endl;
        b << red("DE") << "      :  " << reg(gb.cpu.de) << endl;
        b << red("HL") << "      :  " << reg(gb.cpu.hl) << endl;
        b << red("PC") << "      :  " << reg(gb.cpu.pc) << endl;
        b << red("SP") << "      :  " << reg(gb.cpu.sp) << endl;

        // Flags
        const auto flag = [](bool value) {
            return (value ? value : darkgray(value));
        };

        b << subheader("flags", width) << endl;
        b << red("Z") << " : " << flag(get_bit<Specs::Bits::Flags::Z>(gb.cpu.af)) << "    " << red("N") << " : "
          << flag(get_bit<Specs::Bits::Flags::N>(gb.cpu.af)) << "    " << red("H") << " : "
          << flag(get_bit<Specs::Bits::Flags::H>(gb.cpu.af)) << "    " << red("C") << " : "
          << flag(get_bit<Specs::Bits::Flags::C>(gb.cpu.af)) << endl;

        // Read/Write
        b << subheader("io", width) << endl;

        b << yellow("State") << "   :  " <<
            [this]() {
                switch (gb.cpu.io.state) {
                case Cpu::IO::State::Idle:
                    return darkgray("Idle");
                case Cpu::IO::State::Read:
                    return green("Read");
                case Cpu::IO::State::Write:
                    return red("Write");
                }
                ASSERT_NO_ENTRY();
                return Text {};
            }()
          << endl;
        b << yellow("Data") << "    :  " << bin(gb.cpu.io.data) << " (" << hex(gb.cpu.io.data) << ")" << endl;

        // Interrupts
        const bool IME = gb.cpu.ime == Cpu::ImeState::Enabled;
        const uint8_t IE = gb.interrupts.IE;
        const uint8_t IF = gb.interrupts.IF;

        b << subheader("interrupts", width) << endl;

        b << yellow("State") << "   :  " <<
            [this]() {
                switch (gb.cpu.interrupt.state) {
                case Cpu::InterruptState::None:
                    return darkgray("None");
                case Cpu::InterruptState::Pending:
                    return green("Pending");
                case Cpu::InterruptState::Serving:
                    return green("Serving");
                }
                ASSERT_NO_ENTRY();
                return Text {};
            }()
          << endl;

        if (gb.cpu.interrupt.state == Cpu::InterruptState::Pending) {
            b << yellow("Ticks") << "   :  " << gb.cpu.interrupt.remaining_ticks << endl;
        }

        Text t {hr(width)};

        b << hr(width) << endl;

        b << red("IME") << "     :  " <<
            [this]() {
                switch (gb.cpu.ime) {
                case Cpu::ImeState::Enabled:
                    return cyan("ON");
                case Cpu::ImeState::Requested:
                    return yellow("Requested");
                case Cpu::ImeState::Pending:
                    return yellow("Pending");
                case Cpu::ImeState::Disabled:
                    return darkgray("OFF");
                }
                ASSERT_NO_ENTRY();
                return Text {};
            }()
          << endl;

        b << red("IE ") << "     :  " << bin(IE) << " (" << hex(IE) << ")" << endl;
        b << red("IF ") << "     :  " << bin(IF) << " (" << hex(IF) << ")" << endl;

        const auto intr = [colorbool](bool IME, bool IE, bool IF) {
            return "IE : " + colorbool(IE) + "    IF : " + colorbool(IF) + "    " +
                   ((IME && IE && IF) ? green("ON ") : darkgray("OFF"));
        };

        b << red("VBLANK") << "  :  "
          << intr(IME, test_bit<Specs::Bits::Interrupts::VBLANK>(IE), test_bit<Specs::Bits::Interrupts::VBLANK>(IF))
          << endl;
        b << red("STAT") << "    :  "
          << intr(IME, test_bit<Specs::Bits::Interrupts::STAT>(IE), test_bit<Specs::Bits::Interrupts::STAT>(IF))
          << endl;
        b << red("TIMER") << "   :  "
          << intr(IME, test_bit<Specs::Bits::Interrupts::TIMER>(IE), test_bit<Specs::Bits::Interrupts::TIMER>(IF))
          << endl;
        b << red("JOYPAD") << "  :  "
          << intr(IME, test_bit<Specs::Bits::Interrupts::JOYPAD>(IE), test_bit<Specs::Bits::Interrupts::JOYPAD>(IF))
          << endl;
        b << red("SERIAL") << "  :  "
          << intr(IME, test_bit<Specs::Bits::Interrupts::SERIAL>(IE), test_bit<Specs::Bits::Interrupts::SERIAL>(IF))
          << endl;

        return b;
    };

    // MBC
    const auto make_cartridge_block = [&](uint32_t width) {
        using namespace Specs::Cartridge::Header;
        constexpr uint32_t KB = 1 << 10;
        constexpr uint32_t MB = 1 << 20;

        const auto& cartridge = *gb.cartridge_slot.cartridge;

        const auto& info = backend.get_cartridge_info();
        const auto mbc = info.mbc;
        const auto rom = info.rom;
        const auto ram = info.ram;
        const auto multicart = info.multicart;

        auto b {make_block(width)};

        b << header("CARTRIDGE", width) << endl;

        b << yellow("Title") << "                        :  " << info.title << endl;
        b << yellow("Type") << "                         :  " << [mbc]() -> Text {
            switch (mbc) {
            case Mbc::NO_MBC:
                return "No MBC";
            case Mbc::MBC1:
            case Mbc::MBC1_RAM:
            case Mbc::MBC1_RAM_BATTERY:
                return "MBC1";
            case Mbc::MBC2:
            case Mbc::MBC2_BATTERY:
                return "MBC2";
            case Mbc::ROM_RAM:
            case Mbc::ROM_RAM_BATTERY:
                return "ROMRAM";
            case Mbc::MBC3_TIMER_BATTERY:
            case Mbc::MBC3_TIMER_RAM_BATTERY:
            case Mbc::MBC3:
            case Mbc::MBC3_RAM:
            case Mbc::MBC3_RAM_BATTERY:
                return "MBC3";
            case Mbc::MBC5:
            case Mbc::MBC5_RAM:
            case Mbc::MBC5_RAM_BATTERY:
            case Mbc::MBC5_RUMBLE:
            case Mbc::MBC5_RUMBLE_RAM:
            case Mbc::MBC5_RUMBLE_RAM_BATTERY:
                return "MBC5";
            }
            ASSERT_NO_ENTRY();
            return {};
        }() << endl;

        b << yellow("ROM") << "                          :  " << [rom]() -> Text {
            switch (rom) {
            case Rom::KB_32:
                return "32 KB";
            case Rom::KB_64:
                return "64 KB";
            case Rom::KB_128:
                return "128 KB";
            case Rom::KB_256:
                return "256 KB";
            case Rom::KB_512:
                return "512 KB";
            case Rom::MB_1:
                return "1 MB";
            case Rom::MB_2:
                return "2 MB";
            case Rom::MB_4:
                return "4 MB";
            case Rom::MB_8:
                return "8 MB";
            }
            ASSERT_NO_ENTRY();
            return {};
        }() << endl;

        b << yellow("RAM") << "                          :  " << [ram]() -> Text {
            switch (ram) {
            case Ram::NONE:
                return "None";
            case Ram::KB_2:
                return "2 KB (unofficial)";
            case Ram::KB_8:
                return "8 KB";
            case Ram::KB_32:
                return "32 KB";
            case Ram::KB_128:
                return "128 KB";
            case Ram::KB_64:
                return "64 KB";
            }
            ASSERT_NO_ENTRY();
            return {};
        }() << endl;

        if (mbc != Mbc::NO_MBC) {
            b << subheader("mbc", width) << endl;
        }

        // TODO: more compact version of this bloatware?
        if (mbc == Mbc::MBC1 || mbc == Mbc::MBC1_RAM || mbc == Mbc::MBC1_RAM_BATTERY) {
            const auto mbc1 = [&b](auto&& cart) {
                b << yellow("Banking Mode") << "                 :  " << cart.banking_mode << endl;
                b << yellow("Ram Enabled") << "                  :  " << cart.ram_enabled << endl;
                b << yellow("Upper Rom/Ram Bank Selector") << "  :  "
                  << hex(cart.upper_rom_bank_selector_ram_bank_selector) << endl;
                b << yellow("Rom Bank Selector") << "            :  " << hex(cart.rom_bank_selector) << endl;
            };

            if (mbc == Mbc::MBC1_RAM_BATTERY) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<32 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<32 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<32 * KB, 32 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<64 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<64 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<64 * KB, 32 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<128 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<128 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<128 * KB, 32 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<256 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<256 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<256 * KB, 32 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<512 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<512 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<512 * KB, 32 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::MB_1) {
                    // All the known MBC1M are 1 MB.
                    if (multicart) {
                        if (ram == Ram::NONE) {
                            mbc1(static_cast<const Mbc1M<1 * MB, 0, true>&>(cartridge));
                        } else if (ram == Ram::KB_8) {
                            mbc1(static_cast<const Mbc1M<1 * MB, 8 * KB, true>&>(cartridge));
                        } else if (ram == Ram::KB_32) {
                            mbc1(static_cast<const Mbc1M<1 * MB, 32 * KB, true>&>(cartridge));
                        }
                    } else {
                        if (ram == Ram::NONE) {
                            mbc1(static_cast<const Mbc1<1 * MB, 0, true>&>(cartridge));
                        } else if (ram == Ram::KB_8) {
                            mbc1(static_cast<const Mbc1<1 * MB, 8 * KB, true>&>(cartridge));
                        } else if (ram == Ram::KB_32) {
                            mbc1(static_cast<const Mbc1<1 * MB, 32 * KB, true>&>(cartridge));
                        }
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<2 * MB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<2 * MB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<2 * MB, 32 * KB, true>&>(cartridge));
                    }
                }
            } else {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<32 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<32 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<32 * KB, 32 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<64 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<64 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<64 * KB, 32 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<128 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<128 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<128 * KB, 32 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<256 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<256 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<256 * KB, 32 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<512 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<512 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<512 * KB, 32 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_1) {
                    // All the known MBC1M are 1 MB.
                    if (multicart) {
                        if (ram == Ram::NONE) {
                            mbc1(static_cast<const Mbc1M<1 * MB, 0, false>&>(cartridge));
                        } else if (ram == Ram::KB_8) {
                            mbc1(static_cast<const Mbc1M<1 * MB, 8 * KB, false>&>(cartridge));
                        } else if (ram == Ram::KB_32) {
                            mbc1(static_cast<const Mbc1M<1 * MB, 32 * KB, false>&>(cartridge));
                        }
                    } else {
                        if (ram == Ram::NONE) {
                            mbc1(static_cast<const Mbc1M<1 * MB, 0, false>&>(cartridge));
                        } else if (ram == Ram::KB_8) {
                            mbc1(static_cast<const Mbc1<1 * MB, 8 * KB, false>&>(cartridge));
                        } else if (ram == Ram::KB_32) {
                            mbc1(static_cast<const Mbc1<1 * MB, 32 * KB, false>&>(cartridge));
                        }
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE) {
                        mbc1(static_cast<const Mbc1<2 * MB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc1(static_cast<const Mbc1<2 * MB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc1(static_cast<const Mbc1<2 * MB, 32 * KB, false>&>(cartridge));
                    }
                }
            }
        } else if (mbc == Mbc::MBC2 || mbc == Mbc::MBC2_BATTERY) {
            const auto mbc2 = [&b](auto&& cart) {
                b << yellow("Ram Enabled") << "                  :  " << cart.ram_enabled << endl;
                b << yellow("Rom Bank Selector") << "            :  " << hex(cart.rom_bank_selector) << endl;
            };

            if (mbc == Mbc::MBC2_BATTERY) {
                if (ram == Ram::NONE) {
                    if (rom == Rom::KB_32) {
                        mbc2(static_cast<const Mbc2<32 * KB, true>&>(cartridge));
                    } else if (rom == Rom::KB_64) {
                        mbc2(static_cast<const Mbc2<64 * KB, true>&>(cartridge));
                    } else if (rom == Rom::KB_128) {
                        mbc2(static_cast<const Mbc2<128 * KB, true>&>(cartridge));
                    } else if (rom == Rom::KB_256) {
                        mbc2(static_cast<const Mbc2<256 * KB, true>&>(cartridge));
                    }
                }
            } else {
                if (ram == Ram::NONE) {
                    if (rom == Rom::KB_32) {
                        mbc2(static_cast<const Mbc2<32 * KB, false>&>(cartridge));
                    } else if (rom == Rom::KB_64) {
                        mbc2(static_cast<const Mbc2<64 * KB, false>&>(cartridge));
                    } else if (rom == Rom::KB_128) {
                        mbc2(static_cast<const Mbc2<128 * KB, false>&>(cartridge));
                    } else if (rom == Rom::KB_256) {
                        mbc2(static_cast<const Mbc2<256 * KB, false>&>(cartridge));
                    }
                }
            }
        } else if (mbc == Mbc::MBC3_TIMER_BATTERY || mbc == Mbc::MBC3_TIMER_RAM_BATTERY || mbc == Mbc::MBC3 ||
                   mbc == Mbc::MBC3_RAM || mbc == Mbc::MBC3_RAM_BATTERY) {
            const auto mbc3 = [&b, mbc, subheader, width](auto&& cart) {
                b << yellow("Ram/RTC Enabled") << "              :  " << cart.ram_enabled << endl;
                b << yellow("Rom Bank Selector") << "            :  " << hex(cart.rom_bank_selector) << endl;
                b << yellow("Ram Bank/RTC Reg. Selector") << "   :  "
                  << hex(cart.ram_bank_selector_rtc_register_selector) << endl;

                if (mbc == Mbc::MBC3_TIMER_BATTERY || mbc == Mbc::MBC3_TIMER_RAM_BATTERY) {
                    b << subheader("rtc", width) << endl;
                    b << "                                " << magenta("Latch") << "  |   " << magenta("Real") << endl;
                    b << yellow("Seconds") << "                      :   "
                      << hex((uint8_t)keep_bits<6>(cart.rtc.latched.seconds)) << "    |    "
                      << hex((uint8_t)keep_bits<6>(cart.rtc.real.seconds)) << endl;
                    b << yellow("Minutes") << "                      :   "
                      << hex((uint8_t)keep_bits<6>(cart.rtc.latched.minutes)) << "    |    "
                      << hex((uint8_t)keep_bits<6>(cart.rtc.real.minutes)) << endl;
                    b << yellow("Hours") << "                        :   "
                      << hex((uint8_t)keep_bits<5>(cart.rtc.latched.hours)) << "    |    "
                      << hex((uint8_t)keep_bits<5>(cart.rtc.real.hours)) << endl;
                    b << yellow("Days Low") << "                     :   " << hex(cart.rtc.latched.days.low)
                      << "    |    " << hex(cart.rtc.real.days.low) << endl;
                    b << yellow("Days High") << "                    :   "
                      << hex((uint8_t)get_bits<7, 6, 0>(cart.rtc.latched.days.high)) << "    |    "
                      << hex((uint8_t)get_bits<7, 6, 0>(cart.rtc.real.days.high)) << endl;
                }
            };

            if (mbc == Mbc::MBC3_TIMER_BATTERY || mbc == Mbc::MBC3_TIMER_RAM_BATTERY) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<32 * KB, 0, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<32 * KB, 2 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<32 * KB, 8 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<32 * KB, 32 * KB, true, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<64 * KB, 0, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<64 * KB, 2 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<64 * KB, 8 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<64 * KB, 32 * KB, true, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<128 * KB, 0, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<128 * KB, 2 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<128 * KB, 8 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<128 * KB, 32 * KB, true, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<256 * KB, 0, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<256 * KB, 2 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<256 * KB, 8 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<256 * KB, 32 * KB, true, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<512 * KB, 0, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<512 * KB, 2 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<512 * KB, 8 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<512 * KB, 32 * KB, true, true>&>(cartridge));
                    }
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<1 * MB, 0, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<1 * MB, 2 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<1 * MB, 8 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<1 * MB, 32 * KB, true, true>&>(cartridge));
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<2 * MB, 0, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<2 * MB, 2 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<2 * MB, 8 * KB, true, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<2 * MB, 32 * KB, true, true>&>(cartridge));
                    }
                }
            } else if (mbc == Mbc::MBC3 || mbc == Mbc::MBC3_RAM) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<32 * KB, 0, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<32 * KB, 2 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<32 * KB, 8 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<32 * KB, 32 * KB, false, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<64 * KB, 0, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<64 * KB, 2 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<64 * KB, 8 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<64 * KB, 32 * KB, false, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<128 * KB, 0, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<128 * KB, 2 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<128 * KB, 8 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<128 * KB, 32 * KB, false, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<256 * KB, 0, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<256 * KB, 2 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<256 * KB, 8 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<256 * KB, 32 * KB, false, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<512 * KB, 0, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<512 * KB, 2 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<512 * KB, 8 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<512 * KB, 32 * KB, false, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<1 * MB, 0, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<1 * MB, 2 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<1 * MB, 8 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<1 * MB, 32 * KB, false, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<2 * MB, 0, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<2 * MB, 2 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<2 * MB, 8 * KB, false, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<2 * MB, 32 * KB, false, false>&>(cartridge));
                    }
                }
            } else if (mbc == Mbc::MBC3_RAM_BATTERY) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<32 * KB, 0, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<32 * KB, 2 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<32 * KB, 8 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<32 * KB, 32 * KB, true, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<64 * KB, 0, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<64 * KB, 2 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<64 * KB, 8 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<64 * KB, 32 * KB, true, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<128 * KB, 0, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<128 * KB, 2 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<128 * KB, 8 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<128 * KB, 32 * KB, true, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<256 * KB, 0, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<256 * KB, 2 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<256 * KB, 8 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<256 * KB, 32 * KB, true, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<512 * KB, 0, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<512 * KB, 2 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<512 * KB, 8 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<512 * KB, 32 * KB, true, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<1 * MB, 0, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<1 * MB, 2 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<1 * MB, 8 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<1 * MB, 32 * KB, true, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE) {
                        mbc3(static_cast<const Mbc3<2 * MB, 0, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_2) {
                        mbc3(static_cast<const Mbc3<2 * MB, 2 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc3(static_cast<const Mbc3<2 * MB, 8 * KB, true, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc3(static_cast<const Mbc3<2 * MB, 32 * KB, true, false>&>(cartridge));
                    }
                }
            }
        } else if (mbc == Mbc::MBC5 || mbc == Mbc::MBC5_RAM || mbc == Mbc::MBC5_RUMBLE || mbc == Mbc::MBC5_RUMBLE_RAM ||
                   mbc == Mbc::MBC5_RAM_BATTERY || mbc == Mbc::MBC5_RUMBLE_RAM_BATTERY) {
            const auto mbc5 = [&b](auto&& cart) {
                b << yellow("Ram Enabled") << "                  :  " << cart.ram_enabled << endl;
                b << yellow("Rom Bank Selector") << "            :  " << hex(cart.rom_bank_selector) << endl;
                b << yellow("Ram Bank Selector") << "            :  " << hex(cart.ram_bank_selector) << endl;
            };

            if (mbc == Mbc::MBC5_RAM_BATTERY || mbc == Mbc::MBC5_RUMBLE_RAM_BATTERY) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<32 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<32 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<32 * KB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<32 * KB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<32 * KB, 128 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<64 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<64 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<64 * KB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<64 * KB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<64 * KB, 128 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<128 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<128 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<128 * KB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<128 * KB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<128 * KB, 128 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<256 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<256 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<256 * KB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<256 * KB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<256 * KB, 128 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<512 * KB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<512 * KB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<512 * KB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<512 * KB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<512 * KB, 128 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<1 * MB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<1 * MB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<1 * MB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<1 * MB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<1 * MB, 128 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<2 * MB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<2 * MB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<2 * MB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<2 * MB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<2 * MB, 128 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::MB_4) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<4 * MB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<4 * MB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<4 * MB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<4 * MB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<4 * MB, 128 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::MB_8) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<8 * MB, 0, true>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<8 * MB, 8 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<8 * MB, 32 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<8 * MB, 64 * KB, true>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<8 * MB, 128 * KB, true>&>(cartridge));
                    }
                }
            } else if (mbc == Mbc::MBC5 || mbc == Mbc::MBC5_RAM || mbc == Mbc::MBC5_RUMBLE ||
                       mbc == Mbc::MBC5_RUMBLE_RAM) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<32 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<32 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<32 * KB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<32 * KB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<32 * KB, 128 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<64 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<64 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<64 * KB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<64 * KB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<64 * KB, 128 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<128 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<128 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<128 * KB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<128 * KB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<128 * KB, 128 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<256 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<256 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<256 * KB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<256 * KB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<256 * KB, 128 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<512 * KB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<512 * KB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<512 * KB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<512 * KB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<512 * KB, 128 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<1 * MB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<1 * MB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<1 * MB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<1 * MB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<1 * MB, 128 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<2 * MB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<2 * MB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<2 * MB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<2 * MB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<2 * MB, 128 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_4) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<4 * MB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<4 * MB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<4 * MB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<4 * MB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<4 * MB, 128 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_8) {
                    if (ram == Ram::NONE) {
                        mbc5(static_cast<const Mbc5<8 * MB, 0, false>&>(cartridge));
                    } else if (ram == Ram::KB_8) {
                        mbc5(static_cast<const Mbc5<8 * MB, 8 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_32) {
                        mbc5(static_cast<const Mbc5<8 * MB, 32 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_64) {
                        mbc5(static_cast<const Mbc5<8 * MB, 64 * KB, false>&>(cartridge));
                    } else if (ram == Ram::KB_128) {
                        mbc5(static_cast<const Mbc5<8 * MB, 128 * KB, false>&>(cartridge));
                    }
                }
            }
        }

        return b;
    };

    // PPU
    const auto make_ppu_header = [&](uint32_t width) {
        auto b {make_block(width)};
        b << header("PPU", width) << endl;
        return b;
    };

    const auto make_ppu_general_block_1 = [&](uint32_t width) {
        auto b {make_block(width)};

        // General
        b << yellow("On") << "                :  " << (gb.ppu.lcdc.enable ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("Cycle") << "             :  " << gb.ppu.cycles << endl;
        b << yellow("Dots") << "              :  " << gb.ppu.dots << endl;

        return b;
    };

    const auto make_ppu_general_block_2 = [&](uint32_t width) {
        auto b {make_block(width)};

        // General
        b << yellow("Mode") << "                :  " << [this]() -> Text {
            if (gb.ppu.tick_selector == &Ppu::oam_scan_even || gb.ppu.tick_selector == &Ppu::oam_scan_odd ||
                gb.ppu.tick_selector == &Ppu::oam_scan_done || gb.ppu.tick_selector == &Ppu::oam_scan_after_turn_on)
                return "Oam Scan";
            if (gb.ppu.tick_selector == &Ppu::pixel_transfer_dummy_lx0 ||
                gb.ppu.tick_selector == &Ppu::pixel_transfer_discard_lx0 ||
                gb.ppu.tick_selector == &Ppu::pixel_transfer_discard_lx0_wx0_scx7 ||
                gb.ppu.tick_selector == &Ppu::pixel_transfer_lx0 || gb.ppu.tick_selector == &Ppu::pixel_transfer_lx8) {

                Text t {"Pixel Transfer"};

                std::string block_reason;
                if (gb.ppu.is_fetching_sprite) {
                    block_reason = "fetching sprite";
                } else if (gb.ppu.oam_entries[gb.ppu.lx].is_not_empty()) {
                    block_reason = "sprite hit";
                } else if (gb.ppu.bg_fifo.is_empty()) {
                    block_reason = "empty bg fifo";
                }

                if (!block_reason.empty()) {
                    t += yellow(" [" + block_reason + "]");
                }

                return t;
            }
            if (gb.ppu.tick_selector == &Ppu::hblank || gb.ppu.tick_selector == &Ppu::hblank_453 ||
                gb.ppu.tick_selector == &Ppu::hblank_454 || gb.ppu.tick_selector == &Ppu::hblank_455) {
                return "HBlank";
            }
            if (gb.ppu.tick_selector == &Ppu::hblank_last_line || gb.ppu.tick_selector == &Ppu::hblank_last_line_454 ||
                gb.ppu.tick_selector == &Ppu::hblank_last_line_455) {
                return "HBlank (Last Line)";
            }
            if (gb.ppu.tick_selector == &Ppu::hblank_first_line_after_turn_on) {
                return "HBlank (Glitched Line 0)";
            }
            if (gb.ppu.tick_selector == &Ppu::vblank || gb.ppu.tick_selector == &Ppu::vblank_454) {
                return "VBlank";
            }
            if (gb.ppu.tick_selector == &Ppu::vblank_last_line || gb.ppu.tick_selector == &Ppu::vblank_last_line_2 ||
                gb.ppu.tick_selector == &Ppu::vblank_last_line_7 ||
                gb.ppu.tick_selector == &Ppu::vblank_last_line_454) {
                return "VBlank (Last Line)";
            }

            ASSERT_NO_ENTRY();
            return "Unknown";
        }() << endl;

        b << yellow("Last Stat IRQ") << "       :  " << gb.ppu.last_stat_irq << endl;
        b << yellow("LYC_EQ_LY En.") << "       :  " << gb.ppu.enable_lyc_eq_ly_irq << endl;

        return b;
    };

    const auto make_ppu_block_1 = [&](uint32_t width) {
        auto b {make_block(width)};

#ifdef ENABLE_CGB
        b << subheader("bg palettes", width) << endl;

        const auto bg_palette = [this, &b](uint8_t i) -> Text {
            uint16_t rgb555 = gb.ppu.bg_palettes[2 * i] << 8 | gb.ppu.bg_palettes[2 * i + 1];
            return color(hex(rgb555), RGB555_TO_ANSI_COLORS[rgb555]);
        };

        for (uint8_t i = 0; i < 8; i++) {
            b << yellow("BGP") << yellow(i) << "              : " << bg_palette(i) << " " << bg_palette(i + 1) << " "
              << bg_palette(i + 2) << " " << bg_palette(i + 3) << endl;
        }
#endif

        // LCD
        const auto pixel_color = [this](Lcd::PixelRgb565 lcd_color) {
            return color(hex(lcd_color), RGB565_TO_ANSI_COLORS[lcd_color]);
        };

        b << subheader("lcd", width) << endl;
        b << yellow("X") << "                 :  " << gb.lcd.x << endl;
        b << yellow("Y") << "                 :  " << gb.lcd.y << endl;
        b << yellow("Last Pixels") << "       :  ";
        for (int32_t i = 0; i < 4; i++) {
            int32_t idx = gb.lcd.cursor - 4 + i;
            if (idx >= 0) {
                b << pixel_color(gb.lcd.pixels[idx]) << " ";
            }
        }
        b << endl;

        // BG Fifo
        b << subheader("bg fifo", width) << endl;

        uint8_t bg_pixels[8];
#ifdef ENABLE_CGB
        uint8_t bg_attributes[8];
#endif
        for (uint8_t i = 0; i < gb.ppu.bg_fifo.size(); i++) {
            const Ppu::BgPixel& p = gb.ppu.bg_fifo[i];
            bg_pixels[i] = p.color_index;
#ifdef ENABLE_CGB
            bg_attributes[i] = p.attributes;
#endif
        }

        b << yellow("BG Fifo Pixels") << "    :  " << hex(bg_pixels, gb.ppu.bg_fifo.size()) << endl;
#ifdef ENABLE_CGB
        b << yellow("BG Fifo Attrs.") << "    :  " << hex(bg_attributes, gb.ppu.bg_fifo.size()) << endl;
#endif

        // OBJ Fifo
        uint8_t obj_pixels[8];
        uint8_t obj_attrs[8];
        uint8_t obj_numbers[8];
        uint8_t obj_xs[8];

        for (uint8_t i = 0; i < gb.ppu.obj_fifo.size(); i++) {
            const Ppu::ObjPixel& p = gb.ppu.obj_fifo[i];
            obj_pixels[i] = p.color_index;
            obj_attrs[i] = p.attributes;
            obj_numbers[i] = p.number;
            obj_xs[i] = p.x;
        }

        b << subheader("obj fifo", width) << endl;

        b << yellow("OBJ Fifo Pixels") << "   :  " << hex(obj_pixels, gb.ppu.obj_fifo.size()) << endl;
        b << yellow("OBJ Fifo Number") << "   :  " << hex(obj_numbers, gb.ppu.obj_fifo.size()) << endl;
        b << yellow("OBJ Fifo Attrs.") << "   :  " << hex(obj_attrs, gb.ppu.obj_fifo.size()) << endl;
        b << yellow("OBJ Fifo X") << "        :  " << hex(obj_xs, gb.ppu.obj_fifo.size()) << endl;

        // Window
        b << subheader("window", width) << endl;
        b << yellow("Active for frame") << "  :  " << (gb.ppu.w.active_for_frame ? green("ON") : darkgray("OFF"))
          << endl;
        b << yellow("WLY") << "               :  " << (gb.ppu.w.wly != UINT8_MAX ? gb.ppu.w.wly : darkgray("None"))
          << endl;
        b << yellow("Active") << "            :  " << (gb.ppu.w.active ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("WX Triggers") << "       :  ";
        for (uint8_t i = 0; i < gb.ppu.w.line_triggers.size(); i++) {
            b << Text {gb.ppu.w.line_triggers[i]};
            if (i < gb.ppu.w.line_triggers.size() - 1) {
                b << " | ";
            }
        }
        b << endl;

        // Pixel Transfer
        b << subheader("pixel transfer", width) << endl;

        b << yellow("Fetcher Mode") << "      :  " <<
            [this]() {
                if (gb.ppu.fetcher_tick_selector == &Ppu::bgwin_prefetcher_get_tile_0) {
                    return "BG/WIN Tile0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::bg_prefetcher_get_tile_0) {
                    return "BG Tile0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::bg_prefetcher_get_tile_1) {
                    return "BG Tile1";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::win_prefetcher_activating) {
                    return "WIN Activating";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::win_prefetcher_get_tile_0) {
                    return "WIN Tile0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::win_prefetcher_get_tile_1) {
                    return "WIN Tile1";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::bg_pixel_slice_fetcher_get_tile_data_low_0) {
                    return "BG Low0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::bg_pixel_slice_fetcher_get_tile_data_low_1) {
                    return "BG Low1";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::bg_pixel_slice_fetcher_get_tile_data_high_0) {
                    return "BG High0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::win_pixel_slice_fetcher_get_tile_data_low_0) {
                    return "WIN Low0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::win_pixel_slice_fetcher_get_tile_data_low_1) {
                    return "WIN Low1";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::win_pixel_slice_fetcher_get_tile_data_high_0) {
                    return "WIN High0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::bgwin_pixel_slice_fetcher_get_tile_data_high_1) {
                    return "BG/WIN High1";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::bgwin_pixel_slice_fetcher_push) {
                    return "BG/WIN Push";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::obj_prefetcher_get_tile_0) {
                    return "OBJ Tile0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::obj_prefetcher_get_tile_1) {
                    return "OBJ Tile1";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::obj_pixel_slice_fetcher_get_tile_data_low_0) {
                    return "OBJ Low0";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::obj_pixel_slice_fetcher_get_tile_data_low_1) {
                    return "OBJ Low1";
                }
                if (gb.ppu.fetcher_tick_selector == &Ppu::obj_pixel_slice_fetcher_get_tile_data_high_0) {
                    return "OBJ High0";
                }
                if (gb.ppu.fetcher_tick_selector ==
                    &Ppu::obj_pixel_slice_fetcher_get_tile_data_high_1_and_merge_with_obj_fifo) {
                    return "OBJ High1 & Merge";
                }

                ASSERT_NO_ENTRY();
                return "Unknown";
            }()
          << endl;

        b << yellow("LX") << "                :  " << gb.ppu.lx << endl;
        b << yellow("SCX % 8 Initial") << "   :  " << gb.ppu.pixel_transfer.initial_scx.to_discard << endl;
        b << yellow("SCX % 8 Discard") << "   :  " << gb.ppu.pixel_transfer.initial_scx.discarded << "/"
          << gb.ppu.pixel_transfer.initial_scx.to_discard << endl;
        b << yellow("LX 0->8 Discard") << "   :  " << (gb.ppu.lx < 8 ? (Text(gb.ppu.lx) + "/8") : "8/8") << endl;

        return b;
    };

    const auto make_ppu_block_2 = [&](uint32_t width) {
        auto b {make_block(width)};

#ifdef ENABLE_CGB
        b << subheader("obj palettes", width) << endl;

        const auto obj_palette = [this, &b](uint8_t i) -> Text {
            uint16_t rgb555 = gb.ppu.obj_palettes[2 * i] << 8 | gb.ppu.obj_palettes[2 * i + 1];
            return color(hex(rgb555), RGB555_TO_ANSI_COLORS[rgb555]);
        };

        for (uint8_t i = 0; i < 8; i++) {
            b << yellow("OBJP") << yellow(i) << "               : " << obj_palette(i) << " " << obj_palette(i + 1)
              << " " << obj_palette(i + 2) << " " << obj_palette(i + 3) << endl;
        }
#endif

        // OAM Registers
        b << subheader("oam registers", width) << endl;
        b << yellow("OAM A") << "               :  " << hex(gb.ppu.registers.oam.a) << endl;
        b << yellow("OAM B") << "               :  " << hex(gb.ppu.registers.oam.b) << endl;

        // OAM Scanline entries
        const auto& oam_entries = gb.ppu.scanline_oam_entries;
        b << subheader("oam scanline entries", width) << endl;

        const auto oam_entries_info = [](const auto& v, uint8_t (*fn)(const Ppu::OamScanEntry&)) {
            Text t {};
            for (uint8_t i = 0; i < v.size(); i++) {
                t += Text {fn(v[i])}.rpad(Text::Length {3}) + (i < v.size() - 1 ? " " : "");
            }
            return t;
        };

        b << yellow("OAM Number") << "          :  " << oam_entries_info(oam_entries, [](const Ppu::OamScanEntry& e) {
            return e.number;
        }) << endl;
        b << yellow("OAM X") << "               :  " << oam_entries_info(oam_entries, [](const Ppu::OamScanEntry& e) {
            return e.x;
        }) << endl;
        b << yellow("OAM Y") << "               :  " << oam_entries_info(oam_entries, [](const Ppu::OamScanEntry& e) {
            return e.y;
        }) << endl;

        if (gb.ppu.lx < array_size(gb.ppu.oam_entries)) {
            const auto& oam_entries_hit = gb.ppu.oam_entries[gb.ppu.lx];
            // OAM Hit
            b << subheader("oam hit", width) << endl;

            b << yellow("OAM Number") << "          :  "
              << oam_entries_info(oam_entries_hit,
                                  [](const Ppu::OamScanEntry& e) {
                                      return e.number;
                                  })
              << endl;
            b << yellow("OAM X") << "               :  "
              << oam_entries_info(oam_entries_hit,
                                  [](const Ppu::OamScanEntry& e) {
                                      return e.x;
                                  })
              << endl;
            b << yellow("OAM Y") << "               :  "
              << oam_entries_info(oam_entries_hit,
                                  [](const Ppu::OamScanEntry& e) {
                                      return e.y;
                                  })
              << endl;
        }

        // BG/WIN Prefetcher
        b << subheader("bg/win prefetcher", width) << endl;
        b << yellow("LX") << "                  :  " << gb.ppu.bwf.lx << endl;
        b << yellow("Tile Map X") << "          :  " << gb.ppu.bwf.tilemap_x << endl;
        b << yellow("Tile Map Y") << "          :  " << gb.ppu.bwf.tilemap_y << endl;
        b << yellow("Tile Map Addr.") << "      :  "
          << hex<uint16_t>(Specs::MemoryLayout::VRAM::START + gb.ppu.bwf.tilemap_vram_addr) << endl;
        b << yellow("Tile Map Tile Addr.") << " :  "
          << hex<uint16_t>(Specs::MemoryLayout::VRAM::START + gb.ppu.bwf.tilemap_tile_vram_addr) << endl;

#ifdef ENABLE_CGB
        b << yellow("BG Attributes") << "       :  " << hex<uint8_t>(gb.ppu.bwf.attributes) << endl;
#endif
        b << yellow("Cached Fetch") << "        :  "
          << (gb.ppu.bwf.interrupted_fetch.has_data ? hex<uint16_t>(gb.ppu.bwf.interrupted_fetch.tile_data_high << 8 |
                                                                    gb.ppu.bwf.interrupted_fetch.tile_data_low)
                                                    : darkgray("None"))
          << endl;

        // OBJ Prefetcher
        b << subheader("obj prefetcher", width) << endl;
        b << yellow("OAM Number") << "          :  " << gb.ppu.of.entry.number << endl;
        b << yellow("Tile Number") << "         :  " << gb.ppu.of.tile_number << endl;
        b << yellow("OBJ Attributes") << "      :  " << hex<uint8_t>(gb.ppu.of.attributes) << endl;

        // Pixel Slice Fetcher
        b << subheader("pixel slice fetcher", width) << endl;
        b << yellow("Tile Data Addr.") << "     :  "
          << hex<uint16_t>(Specs::MemoryLayout::VRAM::START + gb.ppu.psf.tile_data_vram_address) << endl;
        b << yellow("Tile Data") << "           :  "
          << hex<uint16_t>(gb.ppu.psf.tile_data_high << 8 | gb.ppu.psf.tile_data_low) << endl;
        b << yellow("Tile Data Ready") << "     :  " << [this]() {
            if (gb.ppu.fetcher_tick_selector == &Ppu::bgwin_pixel_slice_fetcher_push)
                return green("Ready to push");
            if (gb.ppu.fetcher_tick_selector ==
                &Ppu::obj_pixel_slice_fetcher_get_tile_data_high_1_and_merge_with_obj_fifo)
                return green("Ready to merge");
            return darkgray("Not ready");
        }();
        b << endl;

        return b;
    };

#ifdef ENABLE_AUDIO
    // APU
    const auto make_apu_header = [&](uint32_t width) {
        auto b {make_block(width)};
        b << header("APU", width) << endl;
        return b;
    };

    const auto make_apu_general_block_1 = [&](uint32_t width) {
        auto b {make_block(width)};
        b << yellow("On") << "             :  " << (gb.apu.nr52.enable ? green("ON") : darkgray("OFF")) << endl;
        return b;
    };

    const auto make_apu_general_block_2 = [&](uint32_t width) {
        auto b {make_block(width)};
        b << yellow("DIV") << "            :  " << (gb.apu.div_apu % 8) << endl;
        return b;
    };
    const auto make_apu_general_block_3 = [&](uint32_t width) {
        auto b {make_block(width)};
        b << yellow("DIV bit 4") << "      :  " << +gb.apu.prev_div_bit_4 << endl;
        return b;
    };

    const auto make_apu_block_1 = [&](uint32_t width) {
        auto b {make_block(width)};

        b << subheader("channel 1", width) << endl;
        b << yellow("Enabled") << "        :  " << (gb.apu.nr52.ch1 ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("DAC") << "            :  " << (gb.apu.ch1.dac ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("Volume") << "         :  " << +gb.apu.ch1.volume << endl;
        b << yellow("Length Timer") << "   :  " << +gb.apu.ch1.length_timer << endl;
        b << yellow("Trigger Delay") << "  :  " << +gb.apu.ch1.trigger_delay << endl;

        b << subheader2("wave", width) << endl;

        b << yellow("Timer") << "          :  " << +gb.apu.ch1.wave.timer << endl;
        b << yellow("Position") << "       :  " << +gb.apu.ch1.wave.position << endl;

        b << subheader2("volume sweep", width) << endl;
        b << yellow("Timer") << "          :  " << +gb.apu.ch1.volume_sweep.timer << endl;
        b << yellow("Pace") << "           :  " << +gb.apu.ch1.volume_sweep.pace << endl;
        b << yellow("Direction") << "      :  " << +gb.apu.ch1.volume_sweep.direction << endl;

        b << subheader2("period sweep", width) << endl;

        b << yellow("Enabled") << "        :  " << +gb.apu.ch1.period_sweep.enabled << endl;
        b << yellow("Period") << "         :  " << +gb.apu.ch1.period_sweep.period << endl;
        b << yellow("Timer") << "          :  " << +gb.apu.ch1.period_sweep.timer << endl;
        b << yellow("Decreasing") << "     :  " << +gb.apu.ch1.period_sweep.decreasing << endl;

        return b;
    };

    const auto make_apu_block_2 = [&](uint32_t width) {
        auto b {make_block(width)};

        b << subheader("channel 2", width) << endl;
        b << yellow("Enabled") << "        :  " << (gb.apu.nr52.ch2 ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("DAC") << "            :  " << (gb.apu.ch2.dac ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("Volume") << "         :  " << +gb.apu.ch2.volume << endl;
        b << yellow("Length Timer") << "   :  " << +gb.apu.ch2.length_timer << endl;
        b << yellow("Trigger Delay") << "  :  " << +gb.apu.ch2.trigger_delay << endl;

        b << subheader2("wave", width) << endl;

        b << yellow("Timer") << "          :  " << +gb.apu.ch2.wave.timer << endl;
        b << yellow("Position") << "       :  " << +gb.apu.ch2.wave.position << endl;

        b << subheader2("volume sweep", width) << endl;
        b << yellow("Timer") << "          :  " << +gb.apu.ch2.volume_sweep.timer << endl;
        b << yellow("Pace") << "           :  " << +gb.apu.ch2.volume_sweep.pace << endl;
        b << yellow("Direction") << "      :  " << +gb.apu.ch2.volume_sweep.direction << endl;

        return b;
    };

    const auto make_apu_block_3 = [&](uint32_t width) {
        auto b {make_block(width)};

        b << subheader("channel 3", width) << endl;
        b << yellow("Enabled") << "       :  " << (gb.apu.nr52.ch3 ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("DAC") << "           :  " << (gb.apu.nr30.dac ? green("ON") : darkgray("OFF")) << endl;
        b << endl;
        b << yellow("Length Timer") << "  :  " << +gb.apu.ch3.length_timer << endl;
        b << yellow("Trigger Delay") << " :  " << +gb.apu.ch3.trigger_delay << endl;

        b << subheader2("wave", width) << endl;

        b << yellow("Timer") << "         :  " << +gb.apu.ch3.wave.timer << endl;
        b << yellow("Position") << "      :  " << +gb.apu.ch3.wave.position << endl;
        b << yellow("Play Position") << " :  " << +gb.apu.ch3.wave.play_position << endl;
        b << yellow("Last Read") << "     :  " << +gb.apu.ch3.last_read_tick << endl;

        b << subheader2("wave ram", width) << endl;

        b << yellow("Wave[0:3]") << "     :  " << hex<uint8_t>(gb.apu.wave_ram[0]) << hex<uint8_t>(gb.apu.wave_ram[1])
          << hex<uint8_t>(gb.apu.wave_ram[2]) << hex<uint8_t>(gb.apu.wave_ram[3]) << endl;
        b << yellow("Wave[4:7]") << "     :  " << hex<uint8_t>(gb.apu.wave_ram[4]) << hex<uint8_t>(gb.apu.wave_ram[5])
          << hex<uint8_t>(gb.apu.wave_ram[6]) << hex<uint8_t>(gb.apu.wave_ram[7]) << endl;
        b << yellow("Wave[8:11]") << "    :  " << hex<uint8_t>(gb.apu.wave_ram[8]) << hex<uint8_t>(gb.apu.wave_ram[9])
          << hex<uint8_t>(gb.apu.wave_ram[10]) << hex<uint8_t>(gb.apu.wave_ram[11]) << endl;
        b << yellow("Wave[12:15]") << "   :  " << hex<uint8_t>(gb.apu.wave_ram[12]) << hex<uint8_t>(gb.apu.wave_ram[13])
          << hex<uint8_t>(gb.apu.wave_ram[14]) << hex<uint8_t>(gb.apu.wave_ram[15]) << endl;

        return b;
    };

    const auto make_apu_block_4 = [&](uint32_t width) {
        auto b {make_block(width)};

        b << subheader("channel 4", width) << endl;
        b << yellow("Enabled") << "        :  " << (gb.apu.nr52.ch4 ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("DAC") << "            :  " << (gb.apu.ch4.dac ? green("ON") : darkgray("OFF")) << endl;
        b << yellow("Volume") << "         :  " << +gb.apu.ch4.volume << endl;
        b << yellow("Length Timer") << "   :  " << +gb.apu.ch4.length_timer << endl;
        b << endl;

        b << subheader2("wave", width) << endl;

        b << yellow("Timer") << "          :  " << +gb.apu.ch4.wave.timer << endl;
        b << endl;

        b << subheader2("volume sweep", width) << endl;
        b << yellow("Timer") << "          :  " << +gb.apu.ch4.volume_sweep.timer << endl;
        b << yellow("Pace") << "           :  " << +gb.apu.ch4.volume_sweep.pace << endl;
        b << yellow("Direction") << "      :  " << +gb.apu.ch4.volume_sweep.direction << endl;

        b << subheader2("lfsr", width) << endl;

        b << yellow("LFSR") << "           :  " << +gb.apu.ch4.lfsr << endl;

        return b;
    };
#endif

    // Bus
    const auto make_bus_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("BUS", width) << endl;

        const auto bus_request = [](Device::Type dev, uint8_t requests) {
            const uint8_t R = 2 * dev;
            const uint8_t W = 2 * dev + 1;

            if (test_bit(requests, W))
                return red("Write");
            if (test_bit(requests, R))
                return green("Read");
            return darkgray("Idle");
        };

        const auto bus_requests = [bus_request](uint8_t requests) {
            return bus_request(Device::Cpu, requests).lpad(Text::Length {5}) + " | " +
                   bus_request(Device::Dma, requests).lpad(Text::Length {5}) + " | " +
#ifdef ENABLE_CGB
                   bus_request(Device::Hdma, requests).lpad(Text::Length {5}) + " | " +
#endif
                   bus_request(Device::Ppu, requests).lpad(Text::Length {5});
        };

        const auto bus_acquired = [](Device::Type dev, uint8_t acquirers) {
            return test_bit(acquirers, dev) ? green("YES") : darkgray("NO");
        };

        const auto bus_acquirers = [bus_acquired](uint8_t acquirers) {
            return " " + bus_acquired(Device::Cpu, acquirers).lpad(Text::Length {3}) + "  |  " +
                   bus_acquired(Device::Dma, acquirers).lpad(Text::Length {3}) + "  |  " +
#ifdef ENABLE_CGB
                   bus_acquired(Device::Hdma, acquirers).lpad(Text::Length {3}) + "  |  " +
#endif
                   bus_acquired(Device::Ppu, acquirers).lpad(Text::Length {3});
        };

        const auto bus_address = [bus_request](uint16_t address, uint8_t requests) {
            return requests ? hex(address) : darkgray(hex(address));
        };

#ifdef ENABLE_CGB
        b << "                     " << magenta("CPU") << "  |  " << magenta("DMA") << "  |  " << magenta("HDMA")
          << " |  " << magenta("PPU")
#else
        b << "                     " << magenta("CPU") << "  |  " << magenta("DMA") << "  |  " << magenta("PPU")
#endif

          << endl;
        b << cyan("EXT Bus") << "      " << endl;
        b << "  " << yellow("Request") << "   :       " << bus_requests(gb.ext_bus.requests) << endl;
        b << "  " << yellow("Address") << "   :  " << bus_address(gb.ext_bus.address, gb.ext_bus.requests) << endl;
        b << endl;

        b << cyan("CPU Bus") << "      " << endl;
        b << "  " << yellow("Request") << "   :       " << bus_requests(gb.cpu_bus.requests) << endl;
        b << "  " << yellow("Address") << "   :  " << bus_address(gb.cpu_bus.address, gb.cpu_bus.requests) << endl;
        b << endl;

        b << cyan("VRAM Bus") << "      " << endl;
        b << "  " << yellow("Request") << "   :       " << bus_requests(gb.vram_bus.requests) << endl;
        b << "  " << yellow("Acquired") << "  :       " << bus_acquirers(gb.vram_bus.acquirers) << endl;
        b << "  " << yellow("Address") << "   :  " << bus_address(gb.vram_bus.address, gb.vram_bus.requests) << endl;
        b << endl;

        b << cyan("OAM Bus") << "      " << endl;
        b << "  " << yellow("Request") << "   :       " << bus_requests(gb.oam_bus.requests) << endl;
        b << "  " << yellow("Acquired") << "  :       " << bus_acquirers(gb.oam_bus.acquirers) << endl;
        b << "  " << yellow("Address") << "   :  " << bus_address(gb.oam_bus.address, gb.oam_bus.requests) << endl;
        b << endl;

        return b;
    };

    // DMA
    const auto make_dma_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("DMA", width) << endl;

        if (gb.dma.request.state != Dma::RequestState::None || gb.dma.transferring) {
            b << yellow("Request") << "       :  ";
            b <<
                [this]() {
                    switch (gb.dma.request.state) {
                    case Dma::RequestState::Requested:
                        return green("Requested");
                    case Dma::RequestState::Pending:
                        return green("Pending");
                    case Dma::RequestState::None:
                        return darkgray("None");
                    }
                    ASSERT_NO_ENTRY();
                    return Text {};
                }()
              << endl;

            b << yellow("Transfer") << "      :  " << (gb.dma.transferring ? green("In Progress") : darkgray("None"))
              << endl;
            b << yellow("Source") << "        :  " << hex(gb.dma.source) << endl;
            b << yellow("Destination") << "   :  " << hex(gb.dma.source) << endl;
            b << yellow("Progress") << "      :  " << hex<uint16_t>(gb.dma.source + gb.dma.cursor) << " => "
              << hex<uint16_t>(Specs::MemoryLayout::OAM::START + gb.dma.cursor) << " [" << gb.dma.cursor << "/"
              << "159]" << endl;
        } else {
            b << yellow("State") << "         :  " << darkgray("None") << endl;
        }

        return b;
    };

#ifdef ENABLE_CGB
    // HDMA
    const auto make_hdma_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("HDMA", width) << endl;

        if (gb.hdma.request != Hdma::RequestState::None || gb.hdma.pause != Hdma::PauseState::None ||
            gb.hdma.remaining_chunks.state != Hdma::RemainingChunksUpdateState::None || gb.hdma.transferring) {

            b << yellow("Request") << "       :  ";
            b <<
                [this]() {
                    switch (gb.hdma.request) {
                    case Hdma::RequestState::Requested:
                        return green("Requested");
                    case Hdma::RequestState::Pending:
                        return green("Pending");
                    case Hdma::RequestState::None:
                        return darkgray("None");
                    }
                    ASSERT_NO_ENTRY();
                    return Text {};
                }()
              << endl;

            b << yellow("Transfer") << "      :  " << (gb.hdma.transferring ? green("In Progress") : darkgray("None"))
              << endl;

            b << yellow("Mode") << "          :  ";
            b <<
                [this]() {
                    switch (gb.hdma.mode) {
                    case Hdma::TransferMode::GeneralPurpose:
                        return Text {"General Purpose"};
                    case Hdma::TransferMode::HBlank:
                        return Text {"HBlank"};
                    }
                    ASSERT_NO_ENTRY();
                    return Text {};
                }()
              << endl;

            b << yellow("Pause") << "         :  ";
            b <<
                [this]() {
                    switch (gb.hdma.pause) {
                    case Hdma::PauseState::ResumeRequested:
                        return green("Resume Requested");
                    case Hdma::PauseState::ResumePending:
                        return green("Resume Pending");
                    case Hdma::PauseState::None:
                        return darkgray("None");
                    case Hdma::PauseState::Paused:
                        return green("Paused");
                    }
                    ASSERT_NO_ENTRY();
                    return Text {};
                }()
              << endl;

            b << yellow("Source") << "        :  " << hex(gb.hdma.source) << endl;
            b << yellow("Destination") << "   :  " << hex(gb.hdma.destination) << endl;
            b << yellow("Progress") << "      :  " << hex<uint16_t>(gb.hdma.source + gb.hdma.cursor) << " => "
              << hex<uint16_t>(gb.hdma.destination + gb.hdma.cursor) << " [" << gb.hdma.cursor << "/" << gb.hdma.length
              << "]" << endl;

            b << yellow("Chunk Update") << "  :  ";
            b <<
                [this]() {
                    switch (gb.hdma.mode) {
                    case Hdma::RemainingChunksUpdateState::Requested:
                        return green("Requested");
                    case Hdma::RemainingChunksUpdateState::Pending:
                        return green("Pending");
                    case Hdma::RemainingChunksUpdateState::None:
                        return darkgray("None");
                    }
                    ASSERT_NO_ENTRY();
                    return Text {};
                }()
              << endl;
        } else {
            b << yellow("State") << "         :  " << darkgray("None") << endl;
        }

        return b;
    };
#endif

    // Timers
    const auto make_timers_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("TIMERS", width) << endl;
        b << yellow("TMA reload") << " :  " << [this]() -> Text {
            switch (gb.timers.tima_state) {
            case Timers::TimaReloadState::None:
                return darkgray("None");
            case Timers::TimaReloadState::Pending:
                return green("Pending");
            case Timers::TimaReloadState::Reload:
                return green("Reloading");
            default:
                return Text {};
            }
        }() << endl;
        b << hr(width) << endl;

        b << red("DIV (16)") << "   :  " << [this]() -> Text {
            uint16_t div = gb.timers.div16;
            Text t {};
            uint8_t highlight_bit = Specs::Timers::TAC_DIV_BITS_SELECTOR[keep_bits<2>(gb.timers.tac)];
            for (int8_t b = 15; b >= 0; b--) {
                bool high = test_bit(div, b);
                t += ((b == highlight_bit) ? yellow(Text {high}) : Text {high});
                if (b == 8)
                    t += " ";
            }
            t += " (" + hex((uint8_t)(div >> 8)) + " " + hex((uint8_t)(div & 0xFF)) + ")";
            return t;
        }() << endl;

        const auto timer = [](uint8_t value) {
            return Text {bin(value)} + "          (" + hex(value) + ")";
        };

        b << red("DIV") << "        :  " << timer(backend.read_memory(Specs::Registers::Timers::DIV)) << endl;
        b << red("TIMA") << "       :  " << timer(backend.read_memory(Specs::Registers::Timers::TIMA)) << endl;
        b << red("TMA") << "        :  " << timer(backend.read_memory(Specs::Registers::Timers::TMA)) << endl;
        b << red("TAC") << "        :  " << timer(backend.read_memory(Specs::Registers::Timers::TAC)) << endl;

        return b;
    };

    // IO
    const auto make_io_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("IO", width) << endl;

        const auto ios = [this, width](const uint16_t* addresses, uint32_t length) {
            static constexpr uint8_t MNEMONIC_MAX_LENGTH = 6;
            static constexpr uint8_t IO_LENGTH = 4 + MNEMONIC_MAX_LENGTH + 5 + 8 + 2 + 2 + 1 + 4;
            const uint8_t columns = (width + 4) / IO_LENGTH;

            const auto io = [](uint16_t address, uint8_t value) {
                return lightgray(hex(address)) + " " +
                       red(address_to_mnemonic(address)).rpad(Text::Length {MNEMONIC_MAX_LENGTH}) + "  :  " +
                       bin(value) + " (" + hex(value) + ")";
            };

            Text t {};

            uint32_t i;
            for (i = 0; i < length; i++) {
                if (i > 0) {
                    if (i % columns == 0)
                        t += "\n";
                    else
                        t += "    ";
                }

                uint16_t addr = addresses[i];
                uint8_t value = backend.read_memory(addr);
                t += io(addr, value);
            }

            return t;
        };

        b << subheader("joypad", width) << endl;
        b << ios(Specs::Registers::Joypad::REGISTERS, array_size(Specs::Registers::Joypad::REGISTERS)) << endl;
        b << subheader("serial", width) << endl;
        b << ios(Specs::Registers::Serial::REGISTERS, array_size(Specs::Registers::Serial::REGISTERS)) << endl;
        b << subheader("timers", width) << endl;
        b << ios(Specs::Registers::Timers::REGISTERS, array_size(Specs::Registers::Timers::REGISTERS)) << endl;
        b << subheader("interrupts", width) << endl;
        b << ios(Specs::Registers::Interrupts::REGISTERS, array_size(Specs::Registers::Interrupts::REGISTERS)) << endl;
        b << subheader("audio", width) << endl;
        b << ios(Specs::Registers::Sound::REGISTERS, array_size(Specs::Registers::Sound::REGISTERS)) << endl;
        b << subheader("video", width) << endl;
        b << ios(Specs::Registers::Video::REGISTERS, array_size(Specs::Registers::Video::REGISTERS)) << endl;

#ifdef ENABLE_CGB
        b << endl;
        b << header("IO (CGB)", width) << endl;
        b << subheader("audio", width) << endl;
        b << ios(Specs::Registers::Sound::CGB_REGISTERS, array_size(Specs::Registers::Sound::CGB_REGISTERS)) << endl;
        b << subheader("video", width) << endl;
        b << ios(Specs::Registers::Video::CGB_REGISTERS, array_size(Specs::Registers::Video::CGB_REGISTERS)) << endl;
        b << subheader("hdma", width) << endl;
        b << ios(Specs::Registers::Hdma::CGB_REGISTERS, array_size(Specs::Registers::Hdma::CGB_REGISTERS)) << endl;
        b << subheader("banks", width) << endl;
        b << ios(Specs::Registers::Banks::CGB_REGISTERS, array_size(Specs::Registers::Banks::CGB_REGISTERS)) << endl;
        b << subheader("ir", width) << endl;
        b << ios(Specs::Registers::Infrared::CGB_REGISTERS, array_size(Specs::Registers::Infrared::CGB_REGISTERS))
          << endl;
        b << subheader("undocumented", width) << endl;
        b << ios(Specs::Registers::Undocumented::CGB_REGISTERS,
                 array_size(Specs::Registers::Undocumented::CGB_REGISTERS))
          << endl;
#endif
        return b;
    };

    // Code
    const auto make_code_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("CODE", width) << endl;

        if (const auto isr_phase = DebuggerHelpers::get_isr_phase(gb.cpu); isr_phase != std::nullopt) {
            b << yellow("ISR") << "   " << lightgray("M" + std::to_string(*isr_phase + 1) + "/5") << endl;
        } else {

            struct DisassembledInstructionEntry : DisassembledInstructionReference {
                enum class Type { Past, Current, Future, FutureGuess };

                DisassembledInstructionEntry(uint16_t address, const DisassembledInstruction& instruction, Type type) :
                    DisassembledInstructionReference(address, instruction),
                    type(type) {
                }

                DisassembledInstructionEntry(uint16_t address, DisassembledInstruction&& instruction, Type type) :
                    DisassembledInstructionReference(address, std::move(instruction)),
                    type(type) {
                }

                Type type;
            };

            std::list<DisassembledInstructionEntry> code_view;

            const auto current_instruction_opt = backend.disassemble(gb.cpu.instruction.address, true);
            ASSERT(current_instruction_opt);

            const auto& current_instruction = *current_instruction_opt;

            // Past instructions
            {
                uint8_t n = 0;
                for (int32_t addr = current_instruction.address - 1;
                     addr >= 0 && n < auto_disassemble_instructions.past; addr--) {
                    auto instr = backend.get_disassembled_instruction(addr);
                    if (instr) {
                        code_view.emplace_front(static_cast<uint16_t>(addr), *instr,
                                                DisassembledInstructionEntry::Type::Past);
                        n++;
                    }
                }
            }

            // Current instruction
            code_view.emplace_back(current_instruction.address, current_instruction.instruction,
                                   DisassembledInstructionEntry::Type::Current);

            // Next instructions
            {
                uint32_t addr = current_instruction.address + current_instruction.instruction.size();
                for (uint16_t n = 0; n < auto_disassemble_instructions.next && addr <= 0xFFFF; n++) {
                    const auto known_instr = backend.get_disassembled_instruction(addr);
                    auto instr = backend.disassemble(addr);
                    if (!instr)
                        break;
                    const bool known = known_instr && *known_instr == instr->instruction;
                    addr = instr->address + instr->instruction.size();
                    code_view.emplace_back(instr->address, std::move(instr->instruction),
                                           known ? DisassembledInstructionEntry::Type::Future
                                                 : DisassembledInstructionEntry::Type::FutureGuess);
                }
            }

            if (!code_view.empty()) {
                const auto disassembler_entry = [this](const DisassembledInstructionEntry& entry) {
                    Text t {};
                    if (backend.get_breakpoint(entry.address)) {
                        t += red(Text {Token {DOT, 1}});
                    } else {
                        t += " ";
                    }
                    t += " ";

                    Text instr_text {to_string(entry)};

                    if (entry.type == DisassembledInstructionEntry::Type::Current) {
                        instr_text = green(std::move(instr_text));
                    } else if (entry.type == DisassembledInstructionEntry::Type::Past) {
                        instr_text = darkgray(std::move(instr_text));
                    } else if (entry.type == DisassembledInstructionEntry::Type::FutureGuess) {
                        instr_text = lightestgray(std::move(instr_text));
                    }

                    t += instr_text;

                    if (entry.type == DisassembledInstructionEntry::Type::Current) {
                        auto [min, max] = instruction_duration(entry.instruction);
                        if (min) {
                            uint8_t microop_counter = gb.cpu.instruction.cycle_microop;
                            t += "   " +
                                 lightgray("M" + std::to_string(microop_counter + 1) + "/" + std::to_string(min) +
                                           (max != min ? (std::string(":") + std::to_string(+max)) : ""));
                        }
                    }

                    return t;
                };

                // Fill gaps between non-consecutive instructions with ...
                std::list<DisassembledInstructionEntry>::const_iterator last_entry;
                for (auto entry = code_view.begin(); entry != code_view.end(); entry++) {
                    if (entry != code_view.begin() &&
                        last_entry->address + last_entry->instruction.size() < entry->address)
                        b << "  " << darkgray("...") << endl;
                    b << disassembler_entry(*entry) << endl;
                    last_entry = entry;
                }
            }
        }

        return b;
    };

    // Call Stack
    const auto make_call_stack_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("CALL STACK", width) << endl;

        const auto& callstack = backend.get_call_stack();
        for (const auto& entry : callstack) {
            b << to_string(entry) << endl;
        }

        return b;
    };

    // Breakpoints
    const auto make_breakpoints_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("BREAKPOINTS", width) << endl;

        const auto breakpoint = [this](const Breakpoint& b) {
            Text t {"(" + std::to_string(b.id) + ") " + hex(b.address)};
            const auto instr = backend.get_disassembled_instruction(b.address);
            if (instr) {
                t += "  :  " + rpad(hex(*instr), 9) + "   " + rpad(instruction_mnemonic(*instr, b.address), 23);
            }
            return lightmagenta(std::move(t));
        };

        for (const auto& br : backend.get_breakpoints()) {
            b << breakpoint(br) << endl;
        }

        return b;
    };

    // Watchpoints
    const auto make_watchpoints_block = [&](uint32_t width) {
        auto b {make_block(width)};

        const auto watchpoint = [](const Watchpoint& w) {
            Text t {"(" + std::to_string(w.id) + ") " + hex(w.address.from)};

            if (w.address.from != w.address.to)
                t += " - " + std::to_string(w.address.to);

            if (w.condition.enabled) {
                if (w.condition.condition.operation == Watchpoint::Condition::Operator::Equal) {
                    t += " == " + hex(w.condition.condition.operand);
                }
            }

            t += " " + [&w]() -> Text {
                switch (w.type) {
                case Watchpoint::Type::Read:
                    return "(read)";
                case Watchpoint::Type::Write:
                    return "(write)";
                case Watchpoint::Type::ReadWrite:
                    return "(read/write)";
                case Watchpoint::Type::Change:
                    return "(change)";
                }
                return "";
            }();

            return yellow(std::move(t));
        };

        b << header("WATCHPOINTS", width) << endl;
        for (const auto& w : backend.get_watchpoints()) {
            b << watchpoint(w) << endl;
        }

        return b;
    };

    // Display
    const auto make_display_block = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("DISPLAY", width) << endl;

        for (const auto& d : display_entries) {
            b << dump_display_entry(d) << endl;
        }

        return b;
    };

    // Interruption
    const auto make_interruption_block = [&](uint32_t width) {
        auto b {make_block(width)};

        if (std::holds_alternative<ExecutionBreakpointHit>(execution_state)) {
            b << header("INTERRUPTION", width) << endl;
            auto hit = std::get<ExecutionBreakpointHit>(execution_state).breakpoint_hit;
            b << "Triggered breakpoint (" << hit.breakpoint.id << ") at address " << hex(hit.breakpoint.address)
              << endl;
        } else if (std::holds_alternative<ExecutionWatchpointHit>(execution_state)) {
            b << header("INTERRUPTION", width) << endl;
            const auto hit = std::get<ExecutionWatchpointHit>(execution_state).watchpoint_hit;
            b << "Triggered watchpoint (" << hit.watchpoint.id << ") at address " << hex(hit.address) << endl;
            if (hit.access_type == WatchpointHit::AccessType::Read) {
                b << "Read at address " << hex(hit.address) << ": " << hex(hit.new_value) << endl;
            } else {
                b << "Write at address " << hex(hit.address) << endl;
                b << "Old: " << hex(hit.old_value) << endl;
                b << "New: " << hex(hit.new_value) << endl;
            }
        } else if (std::holds_alternative<ExecutionInterrupted>(execution_state)) {
            b << header("INTERRUPTION", width) << endl;
            b << "User interruption request" << endl;
        }

        return b;
    };

    static constexpr uint32_t COLUMN_1_WIDTH = 40;
    auto c1 {make_vertical_layout()};
    c1->add_node(make_gameboy_block(COLUMN_1_WIDTH));
    c1->add_node(make_space_divider());
    c1->add_node(make_cpu_block(COLUMN_1_WIDTH));
    c1->add_node(make_space_divider());
    c1->add_node(make_timers_block(COLUMN_1_WIDTH));

    static constexpr uint32_t COLUMN_2_WIDTH = 49;
    auto c2 {make_vertical_layout()};
    c2->add_node(make_cartridge_block(COLUMN_2_WIDTH));
    c2->add_node(make_space_divider());
    c2->add_node(make_bus_block(COLUMN_2_WIDTH));
    c2->add_node(make_space_divider());
    c2->add_node(make_dma_block(COLUMN_2_WIDTH));
#ifdef ENABLE_CGB
    c2->add_node(make_hdma_block(COLUMN_2_WIDTH));
#endif

    static constexpr uint32_t COLUMN_3_ROW_1_2_PART_1_WIDTH = 45;
    static constexpr uint32_t COLUMN_3_ROW_1_2_PART_2_WIDTH = 52;
    static constexpr uint32_t COLUMN_3_ROW_1_2_WIDTH =
        COLUMN_3_ROW_1_2_PART_1_WIDTH + COLUMN_3_ROW_1_2_PART_2_WIDTH + 1;

    auto c3r1 {make_horizontal_layout()};
    c3r1->add_node(make_ppu_general_block_1(COLUMN_3_ROW_1_2_PART_1_WIDTH));
    c3r1->add_node(make_space_divider());
    c3r1->add_node(make_ppu_general_block_2(COLUMN_3_ROW_1_2_PART_2_WIDTH));

    auto c3r2 {make_horizontal_layout()};
    c3r2->add_node(make_ppu_block_1(COLUMN_3_ROW_1_2_PART_1_WIDTH));
    c3r2->add_node(make_space_divider());
    c3r2->add_node(make_ppu_block_2(COLUMN_3_ROW_1_2_PART_2_WIDTH));

    static constexpr uint32_t COLUMN_3_ROW_3_4_PART_1_WIDTH = 23;
    static constexpr uint32_t COLUMN_3_ROW_3_4_PART_2_WIDTH = 23;
    static constexpr uint32_t COLUMN_3_ROW_3_4_PART_3_WIDTH = 26;
    static constexpr uint32_t COLUMN_3_ROW_3_4_PART_4_WIDTH = 23;

    static constexpr uint32_t COLUMN_3_ROW_3_4_WIDTH = COLUMN_3_ROW_3_4_PART_1_WIDTH + COLUMN_3_ROW_3_4_PART_2_WIDTH +
                                                       COLUMN_3_ROW_3_4_PART_3_WIDTH + COLUMN_3_ROW_3_4_PART_4_WIDTH +
                                                       3;

    static_assert(COLUMN_3_ROW_1_2_WIDTH == COLUMN_3_ROW_3_4_WIDTH);

    static constexpr uint32_t COLUMN_3_WIDTH = COLUMN_3_ROW_1_2_WIDTH;

    auto c3r3 {make_horizontal_layout()};
    c3r3->add_node(make_apu_general_block_1(COLUMN_3_ROW_3_4_PART_1_WIDTH));
    c3r3->add_node(make_space_divider());
    c3r3->add_node(make_apu_general_block_2(COLUMN_3_ROW_3_4_PART_2_WIDTH));
    c3r3->add_node(make_space_divider());
    c3r3->add_node(make_apu_general_block_3(COLUMN_3_ROW_3_4_PART_3_WIDTH));

    auto c3r4 {make_horizontal_layout()};
    c3r4->add_node(make_apu_block_1(COLUMN_3_ROW_3_4_PART_1_WIDTH));
    c3r4->add_node(make_space_divider());
    c3r4->add_node(make_apu_block_2(COLUMN_3_ROW_3_4_PART_2_WIDTH));
    c3r4->add_node(make_space_divider());
    c3r4->add_node(make_apu_block_3(COLUMN_3_ROW_3_4_PART_3_WIDTH));
    c3r4->add_node(make_space_divider());
    c3r4->add_node(make_apu_block_4(COLUMN_3_ROW_3_4_PART_4_WIDTH));

    auto c3 {make_vertical_layout()};
    c3->add_node(make_ppu_header(COLUMN_3_WIDTH));
    c3->add_node(std::move(c3r1));
    c3->add_node(std::move(c3r2));
    c3->add_node(make_space_divider());
    c3->add_node(make_apu_header(COLUMN_3_WIDTH));
    c3->add_node(std::move(c3r3));
    c3->add_node(std::move(c3r4));

    static constexpr uint32_t COLUMN_4_WIDTH = 66;
    auto c4 {make_vertical_layout()};
    c4->add_node(make_io_block(COLUMN_4_WIDTH));

    static constexpr uint32_t FULL_WIDTH = COLUMN_1_WIDTH + COLUMN_2_WIDTH + COLUMN_3_WIDTH + COLUMN_4_WIDTH;

    auto r1 {make_horizontal_layout()};
    r1->add_node(std::move(c1));
    r1->add_node(make_horizontal_line_divider());
    r1->add_node(std::move(c2));
    r1->add_node(make_horizontal_line_divider());
    r1->add_node(std::move(c3));
    r1->add_node(make_horizontal_line_divider());
    r1->add_node(std::move(c4));

    static constexpr uint32_t CODE_WIDTH = 56;
    static constexpr uint32_t CALL_STACK_WIDTH = 36;
    static constexpr uint32_t BREAKPOINTS_WIDTH = 52;
    static constexpr uint32_t WATCHPOINTS_WIDTH = 30;
    static constexpr uint32_t DISPLAY_WIDTH =
        FULL_WIDTH - CODE_WIDTH - CALL_STACK_WIDTH - BREAKPOINTS_WIDTH - WATCHPOINTS_WIDTH - 4;

    auto r2 {make_horizontal_layout()};
    r2->add_node(make_code_block(CODE_WIDTH));
    r2->add_node(make_horizontal_line_divider());
    r2->add_node(make_call_stack_block(CALL_STACK_WIDTH));
    r2->add_node(make_horizontal_line_divider());
    r2->add_node(make_breakpoints_block(BREAKPOINTS_WIDTH));
    r2->add_node(make_horizontal_line_divider());
    r2->add_node(make_watchpoints_block(WATCHPOINTS_WIDTH));
    r2->add_node(make_horizontal_line_divider());
    r2->add_node(make_display_block(DISPLAY_WIDTH));

    auto root {make_vertical_layout()};
    root->add_node(std::move(r1));
    root->add_node(make_vertical_line_divider());
    root->add_node(std::move(r2));
    auto interruption_block {make_interruption_block(FULL_WIDTH)};
    if (!interruption_block->lines.empty()) {
        root->add_node(make_interruption_block(FULL_WIDTH));
    }

    Presenter(std::cout).present(*root);
}

std::string DebuggerFrontend::dump_memory(uint16_t from, uint32_t n, MemoryOutputFormat fmt,
                                          std::optional<uint8_t> fmt_arg, bool raw) const {
    std::string s;

    const auto read_memory_func = raw ? &DebuggerBackend::read_memory_raw : &DebuggerBackend::read_memory;

    if (fmt == MemoryOutputFormat::Hexadecimal) {
        for (uint32_t i = 0; i < n; i++) {
            s += hex((backend.*read_memory_func)(from + i)) + " ";
        }
    } else if (fmt == MemoryOutputFormat::Binary) {
        for (uint32_t i = 0; i < n; i++) {
            s += bin((backend.*read_memory_func)(from + i)) + " ";
        }
    } else if (fmt == MemoryOutputFormat::Decimal) {
        for (uint32_t i = 0; i < n; i++) {
            s += std::to_string((backend.*read_memory_func)(from + i)) + " ";
        }
    } else if (fmt == MemoryOutputFormat::Instruction) {
        backend.disassemble_multi(from, n, true);
        uint32_t i = 0;
        for (uint32_t address = from; address <= 0xFFFF && i < n;) {
            std::optional<DisassembledInstruction> disas = backend.get_disassembled_instruction(address);
            if (!disas)
                FATAL("Failed to disassemble at address " + std::to_string(address));

            s += to_string(DisassembledInstructionReference {static_cast<uint16_t>(address), *disas}) +
                 ((i < n - 1) ? "\n" : "");
            address += disas->size();
            i++;
        }
    } else if (fmt == MemoryOutputFormat::Hexdump) {
        std::vector<uint8_t> data;
        for (uint32_t i = 0; i < n; i++) {
            data.push_back((backend.*read_memory_func)(from + i));
        }
        uint8_t cols = fmt_arg ? *fmt_arg : 16;
        s += Hexdump().base_address(from).show_addresses(true).show_ascii(false).num_columns(cols).hexdump(data.data(),
                                                                                                           data.size());
    }
    return s;
}

std::string DebuggerFrontend::dump_display_entry(const DebuggerFrontend::DisplayEntry& d) const {
    std::stringstream ss;
    if (std::holds_alternative<DisplayEntry::Examine>(d.expression)) {
        DisplayEntry::Examine dx = std::get<DisplayEntry::Examine>(d.expression);
        ss << d.id << ": "
           << "x" << (dx.raw ? "x" : "") << "/" << dx.length << static_cast<char>(dx.format)
           << (dx.format_arg ? std::to_string(*dx.format_arg) : "") << " " << hex(dx.address) << std::endl;
        ss << dump_memory(dx.address, dx.length, dx.format, dx.format_arg, dx.raw);
    }
    return ss.str();
}