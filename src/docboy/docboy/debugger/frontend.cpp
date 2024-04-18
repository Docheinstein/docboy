#include "frontend.h"
#include "backend.h"
#include "docboy/cartridge/mbc1/mbc1.h"
#include "docboy/cartridge/mbc1/mbc1m.h"
#include "docboy/cartridge/mbc2/mbc2.h"
#include "docboy/cartridge/mbc3/mbc3.h"
#include "docboy/cartridge/mbc5/mbc5.h"
#include "docboy/cartridge/nombc/nombc.h"
#include "docboy/core/core.h"
#include "helpers.h"
#include "mnemonics.h"
#include "shared.h"
#include "tui/block.h"
#include "tui/decorators.h"
#include "tui/divider.h"
#include "tui/factory.h"
#include "tui/hlayout.h"
#include "tui/presenter.h"
#include "tui/vlayout.h"
#include "utils/formatters.hpp"
#include "utils/hexdump.hpp"
#include "utils/memory.h"
#include "utils/strings.hpp"
#include <csignal>
#include <iostream>
#include <list>
#include <optional>
#include <regex>
#include <utility>

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
    } address {};
    struct {
        bool enabled {};
        Watchpoint::Condition condition {};
    } condition {};
};

struct FrontendDeleteCommand {
    std::optional<uint16_t> num {};
};

struct FrontendAutoDisassembleCommand {
    uint16_t n {};
};

struct FrontendExamineCommand {
    MemoryOutputFormat format {};
    std::optional<uint8_t> formatArg {};
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
    std::optional<uint8_t> formatArg {};
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

struct FrontendContinueCommand {};

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
    char* endPtr;
    T result = std::strtol(cs, &endPtr, 16);
    if (endPtr == cs || *endPtr != '\0') {
        if (ok)
            *ok = false;
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
    if (std::optional<uint16_t> addr = mnemonic_to_address(s))
        return *addr;
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

         if (access == "r")
             cmd.type = Watchpoint::Type::Read;
         else if (access == "a")
             cmd.type = Watchpoint::Type::ReadWrite;
         else
             cmd.type = Watchpoint::Type::Change;

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

         if (access == "r")
             cmd.type = Watchpoint::Type::Read;
         else if (access == "a")
             cmd.type = Watchpoint::Type::ReadWrite;
         else
             cmd.type = Watchpoint::Type::Change;

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
         if (!num.empty())
             cmd.num = std::stoi(num);
         return cmd;
     }},
    {std::regex(R"(ad\s*(\d+)?)"), "ad <num>", "Automatically disassemble next <n> instructions (default = 10)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         FrontendAutoDisassembleCommand cmd {};
         const std::string& n = groups[0];
         cmd.n = !n.empty() ? std::stoi(n) : 10;
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
         const std::string& formatArg = groups[3];
         const std::string& address = groups[4];
         cmd.raw = !raw.empty();
         cmd.length = length.empty() ? 1 : std::stoi(length);
         cmd.format = MemoryOutputFormat(format.empty() ? 'x' : format[0]);
         if (!formatArg.empty())
             cmd.formatArg = stoi(formatArg);
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
         const std::string& formatArg = groups[3];
         const std::string& address = groups[4];
         cmd.raw = !raw.empty();
         cmd.length = length.empty() ? 1 : std::stoi(length);
         cmd.format = MemoryOutputFormat(format.empty() ? 'x' : format[0]);
         if (!formatArg.empty())
             cmd.formatArg = stoi(formatArg);
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
    {std::regex(R"(c)"), "c", "Continue",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         return FrontendContinueCommand {};
     }},
    {std::regex(R"(trace\s*(\d+)?)"), "trace [<level>]", "Set the trace level or toggle it (output on stderr)",
     [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
         const std::string& level = groups[0];
         FrontendTraceCommand cmd;
         if (!level.empty())
             cmd.level = std::stoi(level);
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
    backend(backend),
    core(backend.getCore()),
    gb(core.gb) {
    attach_sigint_handler();
}

DebuggerFrontend::~DebuggerFrontend() {
    detach_sigint_handler();
}

// ============================= COMMANDS ======================================

// Breakpoint
template <>
std::optional<Command>
DebuggerFrontend::handleCommand<FrontendBreakpointCommand>(const FrontendBreakpointCommand& cmd) {
    uint32_t id = backend.addBreakpoint(cmd.address);
    std::cout << "Breakpoint [" << id << "] at " << hex(cmd.address) << std::endl;
    reprintUI = true;
    return std::nullopt;
}

// Watchpoint
template <>
std::optional<Command>
DebuggerFrontend::handleCommand<FrontendWatchpointCommand>(const FrontendWatchpointCommand& cmd) {
    std::optional<Watchpoint::Condition> cond;
    if (cmd.condition.enabled)
        cond = cmd.condition.condition;
    uint32_t id = backend.addWatchpoint(cmd.type, cmd.address.from, cmd.address.to, cond);
    if (cmd.address.from == cmd.address.to)
        std::cout << "Watchpoint [" << id << "] at " << hex(cmd.address.from) << std::endl;
    else
        std::cout << "Watchpoint [" << id << "] at " << hex(cmd.address.from) << " - " << hex(cmd.address.to)
                  << std::endl;
    reprintUI = true;
    return std::nullopt;
}

// Delete
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendDeleteCommand>(const FrontendDeleteCommand& cmd) {
    if (cmd.num)
        backend.removePoint(*cmd.num);
    else
        backend.clearPoints();
    reprintUI = true;
    return std::nullopt;
}

// AutoDisassemble
template <>
std::optional<Command>
DebuggerFrontend::handleCommand<FrontendAutoDisassembleCommand>(const FrontendAutoDisassembleCommand& cmd) {
    autoDisassembleNextInstructions = cmd.n;
    reprintUI = true;
    return std::nullopt;
}

// Examine
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendExamineCommand>(const FrontendExamineCommand& cmd) {
    std::cout << dumpMemory(cmd.address, cmd.length, cmd.format, cmd.formatArg, cmd.raw) << std::endl;
    return std::nullopt;
}

// SearchBytes
template <>
std::optional<Command>
DebuggerFrontend::handleCommand<FrontendSearchBytesCommand>(const FrontendSearchBytesCommand& cmd) {
    // TODO: handle bytes in different MBC banks?

    // read all memory
    std::vector<uint8_t> mem {};
    mem.resize(0x10000);
    for (uint32_t addr = 0; addr <= 0xFFFF; addr++) {
        mem[addr] = backend.readMemory(addr);
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
DebuggerFrontend::handleCommand<FrontendSearchInstructionsCommand>(const FrontendSearchInstructionsCommand& cmd) {
    // find instruction among known disassembled instructions
    for (const auto& [addr, instr] : backend.getDisassembledInstructions()) {
        if (mem_find_first(instr.data(), instr.size(), cmd.instruction.data(), cmd.instruction.size())) {
            std::cout << hex(addr) << "    " << instruction_mnemonic(instr, addr) << std::endl;
        }
    }
    return std::nullopt;
}

// Display
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendDisplayCommand>(const FrontendDisplayCommand& cmd) {
    DisplayEntry d = {static_cast<uint32_t>(displayEntries.size()),
                      DisplayEntry::Examine {cmd.format, cmd.formatArg, cmd.address, cmd.length, cmd.raw}};
    displayEntries.push_back(d);
    std::cout << dumpDisplayEntry(d);
    reprintUI = true;
    return std::nullopt;
}

// Undisplay
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendUndisplayCommand>(const FrontendUndisplayCommand& cmd) {
    displayEntries.clear();
    reprintUI = true;
    return std::nullopt;
}

// Tick
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendTickCommand>(const FrontendTickCommand& cmd) {
    return TickCommand {cmd.count};
}

// Dot
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendDotCommand>(const FrontendDotCommand& cmd) {
    return DotCommand {cmd.count};
}

// Step
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendStepCommand>(const FrontendStepCommand& cmd) {
    return StepCommand {cmd.count};
}

// MicroStep
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendMicroStepCommand>(const FrontendMicroStepCommand& cmd) {
    return MicroStepCommand {cmd.count};
}

// Next
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendNextCommand>(const FrontendNextCommand& cmd) {
    return NextCommand {cmd.count};
}

// MicroNext
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendMicroNextCommand>(const FrontendMicroNextCommand& cmd) {
    return MicroNextCommand {cmd.count};
}

// Frame
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendFrameCommand>(const FrontendFrameCommand& cmd) {
    return FrameCommand {cmd.count};
}

// FrameBack
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendFrameBackCommand>(const FrontendFrameBackCommand& cmd) {
    return FrameBackCommand {cmd.count};
}

// Scanline
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendScanlineCommand>(const FrontendScanlineCommand& cmd) {
    return ScanlineCommand {cmd.count};
}

// Continue
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendContinueCommand>(const FrontendContinueCommand& cmd) {
    return ContinueCommand();
}

// Trace
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendTraceCommand>(const FrontendTraceCommand& cmd) {
    if (cmd.level)
        trace = *cmd.level;
    else
        trace = trace ? 0 : MaxTraceFlag;

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
DebuggerFrontend::handleCommand<FrontendDumpDisassembleCommand>(const FrontendDumpDisassembleCommand& cmd) {
    std::vector<DisassembledInstructionReference> disassembled = backend.getDisassembledInstructions();
    for (uint32_t i = 0; i < disassembled.size(); i++) {
        const auto& instr = disassembled[i];
        std::cerr << to_string(instr) << std::endl;
        if (i < disassembled.size() - 1 && instr.address + instr.instruction.size() != disassembled[i + 1].address)
            std::cerr << std::endl; // next instruction is not adjacent to this one
    }
    std::cout << "Dumped " << disassembled.size() << " instructions" << std::endl;

    return std::nullopt;
}

// Help
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendHelpCommand>(const FrontendHelpCommand& cmd) {
    auto it = std::max_element(std::begin(FRONTEND_COMMANDS), std::end(FRONTEND_COMMANDS),
                               [](const FrontendCommandInfo& i1, const FrontendCommandInfo& i2) {
                                   return i1.format.length() < i2.format.length();
                               });
    const int longestCommandFormat = static_cast<int>(it->format.length());
    for (const auto& info : FRONTEND_COMMANDS) {
        std::cout << std::left << std::setw(longestCommandFormat) << info.format << " : " << info.help << std::endl;
    }
    return std::nullopt;
}

// Quit
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendQuitCommand>(const FrontendQuitCommand& cmd) {
    return AbortCommand();
}

Command DebuggerFrontend::pullCommand(const ExecutionState& executionState) {
    using namespace Tui;

    std::optional<Command> commandToSend {};
    std::string cmdline;

    reprintUI = true;

    // Pull command loop
    do {
        if (reprintUI) {
            reprintUI = false;
            printUI(executionState);
        }

        // Prompt
        std::cout << bold(green(">>")).str() << std::endl;

        // Eventually notify observers of this frontend that a command
        // is about to be pulled
        if (onPullingCommandCallback)
            onPullingCommandCallback();

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

        if (cmdline.empty())
            cmdline = lastCmdline;
        else
            lastCmdline = cmdline;

        // Eventually notify observers of this frontend of the pulled command
        if (onCommandPulledCallback) {
            if (onCommandPulledCallback(cmdline)) {
                // handled by the observer
                continue;
            }
        }

        // Parse command
        std::optional<FrontendCommand> parseResult = parse_cmdline(cmdline);
        if (!parseResult) {
            std::cout << "Invalid command" << std::endl;
            continue;
        }

        const FrontendCommand& cmd = *parseResult;

        if (std::holds_alternative<FrontendBreakpointCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendBreakpointCommand>(cmd));
        } else if (std::holds_alternative<FrontendWatchpointCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendWatchpointCommand>(cmd));
        } else if (std::holds_alternative<FrontendDeleteCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendDeleteCommand>(cmd));
        } else if (std::holds_alternative<FrontendAutoDisassembleCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendAutoDisassembleCommand>(cmd));
        } else if (std::holds_alternative<FrontendExamineCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendExamineCommand>(cmd));
        } else if (std::holds_alternative<FrontendSearchBytesCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendSearchBytesCommand>(cmd));
        } else if (std::holds_alternative<FrontendSearchInstructionsCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendSearchInstructionsCommand>(cmd));
        } else if (std::holds_alternative<FrontendDisplayCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendDisplayCommand>(cmd));
        } else if (std::holds_alternative<FrontendUndisplayCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendUndisplayCommand>(cmd));
        } else if (std::holds_alternative<FrontendTickCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendTickCommand>(cmd));
        } else if (std::holds_alternative<FrontendDotCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendDotCommand>(cmd));
        } else if (std::holds_alternative<FrontendStepCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendStepCommand>(cmd));
        } else if (std::holds_alternative<FrontendMicroStepCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendMicroStepCommand>(cmd));
        } else if (std::holds_alternative<FrontendNextCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendNextCommand>(cmd));
        } else if (std::holds_alternative<FrontendMicroNextCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendMicroNextCommand>(cmd));
        } else if (std::holds_alternative<FrontendFrameCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendFrameCommand>(cmd));
        } else if (std::holds_alternative<FrontendFrameBackCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendFrameBackCommand>(cmd));
        } else if (std::holds_alternative<FrontendScanlineCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendScanlineCommand>(cmd));
        } else if (std::holds_alternative<FrontendContinueCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendContinueCommand>(cmd));
        } else if (std::holds_alternative<FrontendTraceCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendTraceCommand>(cmd));
        } else if (std::holds_alternative<FrontendDumpDisassembleCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendDumpDisassembleCommand>(cmd));
        } else if (std::holds_alternative<FrontendHelpCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendHelpCommand>(cmd));
        } else if (std::holds_alternative<FrontendQuitCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendQuitCommand>(cmd));
        }
    } while (!commandToSend);

    return *commandToSend;
}

void DebuggerFrontend::onTick(uint64_t tick) {
    if (trace) {
        const Cpu& cpu = gb.cpu;

        const bool mTrace = (trace & TraceFlagMCycle) || (trace & TraceFlagTCycle);
        const bool tTrace = (trace & TraceFlagTCycle);
        const bool isMcycle = tick % 4 == 0;
        const bool isNewInstruction = cpu.instruction.microop.counter == 0;

        if (!(tTrace || (isMcycle && mTrace) || (isMcycle && isNewInstruction))) {
            // do not trace
            return;
        }

        std::cerr << std::left << std::setw(12) << "[" + std::to_string(tick) + "] ";

        if (trace & TraceFlagInstruction) {
            std::string instrStr;
            if (cpu.halted) {
                instrStr = "<HALTED>";
            } else {
                if (DebuggerHelpers::isInIsr(cpu)) {
                    instrStr = "ISR " + hex(cpu.instruction.address);
                } else {
                    const auto instr = backend.disassemble(cpu.instruction.address);
                    if (instr)
                        instrStr = instruction_mnemonic(instr->instruction, cpu.instruction.address);
                    else
                        instrStr = "unknown";
                }
            }

            std::cerr << std::left << std::setw(28) << instrStr << "  ";
        }

        if (trace & TraceFlagRegisters) {
            std::cerr << "AF:" << hex(cpu.AF) << " BC:" << hex(cpu.BC) << " DE:" << hex(cpu.DE) << " HL:" << hex(cpu.HL)
                      << " SP:" << hex(cpu.SP) << " PC:" << hex((uint16_t)(cpu.PC)) << "  ";
        }

        if (trace & TraceFlagInterrupts) {
            const InterruptsIO& interrupts = gb.interrupts;
            std::cerr << "IME:"
                      << (cpu.IME == Cpu::ImeState::Enabled
                              ? "1"
                              : ((cpu.IME == Cpu::ImeState::Pending || cpu.IME == Cpu::ImeState::Requested) ? "!"
                                                                                                            : "0"))
                      << " IE:" << hex((uint8_t)interrupts.IE) << " IF:" << hex((uint8_t)interrupts.IF) << "  ";
        }

        if (trace & TraceFlagDma) {
            std::cerr << "DMA:" << std::setw(3) << (gb.dma.transferring ? std::to_string(gb.dma.cursor) : "OFF")
                      << "  ";
        }

        if (trace & TraceFlagTimers) {
            std::cerr << "DIV16:" << hex(gb.timers.DIV)
                      << " DIV:" << hex(backend.readMemory(Specs::Registers::Timers::DIV))
                      << " TIMA:" << hex(backend.readMemory(Specs::Registers::Timers::TIMA))
                      << " TMA:" << hex(backend.readMemory(Specs::Registers::Timers::TMA))
                      << " TAC:" << hex(backend.readMemory(Specs::Registers::Timers::TAC)) << "  ";
        }

        if (trace & TraceFlagStack) {
            std::cerr << "[SP+1]:" << hex(backend.readMemory(cpu.SP + 1)) << " "
                      << "[SP]:" << hex(backend.readMemory(cpu.SP)) << " "
                      << "[SP-1]:" << hex(backend.readMemory(cpu.SP - 1)) << "  ";
        }

        if (trace & TraceFlagMemory) {
            std::cerr << "[BC]:" << hex(backend.readMemory(cpu.BC)) << " "
                      << "[DE]:" << hex(backend.readMemory(cpu.DE)) << " "
                      << "[HL]:" << hex(backend.readMemory(cpu.HL)) << "  ";
        }

        if (trace & TraceFlagPpu) {
            const Ppu& ppu = gb.ppu;
            std::cerr << "Mode:" << +keep_bits<2>(gb.video.STAT) << " Dots:" << std::setw(12) << ppu.dots << "  ";
        }

        if (trace & TraceFlagHash) {
            std::cerr << "Hash:" << hex(static_cast<uint16_t>(backend.computeStateHash())) << "  ";
        }

        std::cerr << std::endl;
    }

    if (sigint_trigger) {
        sigint_trigger = 0;
        backend.interrupt();
    }
}

void DebuggerFrontend::setOnPullingCommandCallback(std::function<void()> callback) {
    onPullingCommandCallback = std::move(callback);
}

void DebuggerFrontend::setOnCommandPulledCallback(std::function<bool(const std::string&)> callback) {
    onCommandPulledCallback = std::move(callback);
}

// ================================= UI ========================================

void DebuggerFrontend::printUI(const ExecutionState& executionState) const {
    using namespace Tui;

    const auto title = [](const Text& text, uint16_t width, const std::string& sep = DASH) {
        check(width >= text.size());
        Text t {text.size() > 0 ? (" " + text + " ") : text};

        Token sepToken {sep, true};

        const uint16_t w1 = (width - t.size()) / 2;
        Text pre;
        for (uint16_t i = 0; i < w1; i++)
            pre += sepToken;

        const uint16_t w2 = width - t.size() - w1;
        Text post;
        for (uint16_t i = 0; i < w2; i++)
            post += sepToken;

        return darkgray(std::move(pre)) + t + darkgray(std::move(post));
    };

    const auto header = [title](const char* c, uint16_t width) {
        return title(bold(lightcyan(c)), width);
    };

    const auto subheader = [title](const char* c, uint16_t width) {
        return title(cyan(c), width);
    };

    const auto hr = [title](uint16_t width) {
        return title("", width);
    };

    const auto colorbool = [](bool b) -> Text {
        return b ? b : gray(b);
    };

    const auto makeHorizontalLineDivider = []() {
        return make_divider(Text {} + "  " + Token {PIPE, 1} + "  ");
    };

    const auto makeVerticalLineDivider = []() {
        return make_divider(Text {Token {DASH, 1}});
    };

    const auto makeSpaceDivider = []() {
        return make_divider(" ");
    };

    // Gameboy
    const auto makeGameboyBlock = [&](uint32_t width) {
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

        b << yellow("Hash") << "     :  " << hex(static_cast<uint16_t>(backend.computeStateHash())) << endl;

        return b;
    };

    // CPU
    const auto makeCpuBlock = [&](uint32_t width) {
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
        b << red("AF") << "      :  " << reg(gb.cpu.AF) << endl;
        b << red("BC") << "      :  " << reg(gb.cpu.BC) << endl;
        b << red("DE") << "      :  " << reg(gb.cpu.DE) << endl;
        b << red("HL") << "      :  " << reg(gb.cpu.HL) << endl;
        b << red("PC") << "      :  " << reg(gb.cpu.PC) << endl;
        b << red("SP") << "      :  " << reg(gb.cpu.SP) << endl;

        // Flags
        const auto flag = [](bool value) {
            return (value ? value : darkgray(value));
        };

        b << subheader("flags", width) << endl;
        b << red("Z") << " : " << flag(get_bit<Specs::Bits::Flags::Z>(gb.cpu.AF)) << "    " << red("N") << " : "
          << flag(get_bit<Specs::Bits::Flags::N>(gb.cpu.AF)) << "    " << red("H") << " : "
          << flag(get_bit<Specs::Bits::Flags::H>(gb.cpu.AF)) << "    " << red("C") << " : "
          << flag(get_bit<Specs::Bits::Flags::C>(gb.cpu.AF)) << endl;

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
                checkNoEntry();
                return Text {};
            }()
          << endl;
        b << yellow("Data") << "    :  " << bin(gb.cpu.io.data) << " (" << hex(gb.cpu.io.data) << ")" << endl;

        // Interrupts
        const bool IME = gb.cpu.IME == Cpu::ImeState::Enabled;
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
                checkNoEntry();
                return Text {};
            }()
          << endl;

        if (gb.cpu.interrupt.state == Cpu::InterruptState::Pending) {
            b << yellow("Ticks") << "   :  " << gb.cpu.interrupt.remainingTicks << endl;
        }

        Text t {hr(width)};

        b << hr(width) << endl;

        b << red("IME") << "     :  " <<
            [this]() {
                switch (gb.cpu.IME) {
                case Cpu::ImeState::Enabled:
                    return cyan("ON");
                case Cpu::ImeState::Requested:
                    return yellow("Requested");
                case Cpu::ImeState::Pending:
                    return yellow("Pending");
                case Cpu::ImeState::Disabled:
                    return darkgray("OFF");
                }
                checkNoEntry();
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
    const auto makeCartridgeBlock = [&](uint32_t width) {
        using namespace Specs::Cartridge::Header;
        constexpr uint32_t KB = 1 << 10;
        constexpr uint32_t MB = 1 << 20;

        const auto& cartridge = *gb.cartridgeSlot.cartridge;

        const auto& info = backend.getCartridgeInfo();
        const auto mbc = info.mbc;
        const auto rom = info.rom;
        const auto ram = info.ram;
        const auto multicart = info.multicart;

        auto b {make_block(width)};

        b << header("CARTRIDGE", width) << endl;

        b << yellow("Type") << "                        :  " << [mbc]() -> Text {
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
            checkNoEntry();
            return {};
        }() << endl;

        b << yellow("ROM") << "                         :  " << [rom]() -> Text {
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
            checkNoEntry();
            return {};
        }() << endl;

        b << yellow("RAM") << "                         :  " << [ram]() -> Text {
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
            checkNoEntry();
            return {};
        }() << endl;

        if (mbc != Mbc::NO_MBC)
            b << subheader("mbc", width) << endl;

        // TODO: more compact version of this bloatware?
        if (mbc == Mbc::MBC1 || mbc == Mbc::MBC1_RAM || mbc == Mbc::MBC1_RAM_BATTERY) {
            const auto mbc1 = [&b](auto&& cart) {
                b << yellow("Banking Mode") << "                 :  " << cart.bankingMode << endl;
                b << yellow("Ram Enabled") << "                  :  " << cart.ramEnabled << endl;
                b << yellow("Upper Rom/Ram Bank Selector") << "  :  " << hex(cart.upperRomBankSelector_ramBankSelector)
                  << endl;
                b << yellow("Rom Bank Selector") << "            :  " << hex(cart.romBankSelector) << endl;
            };

            if (mbc == Mbc::MBC1_RAM_BATTERY) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<32 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<32 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<32 * KB, 32 * KB, true>&>(cartridge));
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<64 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<64 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<64 * KB, 32 * KB, true>&>(cartridge));
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<128 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<128 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<128 * KB, 32 * KB, true>&>(cartridge));
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<256 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<256 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<256 * KB, 32 * KB, true>&>(cartridge));
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<512 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<512 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<512 * KB, 32 * KB, true>&>(cartridge));
                } else if (rom == Rom::MB_1) {
                    // All the known MBC1M are 1 MB.
                    if (multicart) {
                        if (ram == Ram::NONE)
                            mbc1(static_cast<const Mbc1M<1 * MB, 0, true>&>(cartridge));
                        else if (ram == Ram::KB_8)
                            mbc1(static_cast<const Mbc1M<1 * MB, 8 * KB, true>&>(cartridge));
                        else if (ram == Ram::KB_32)
                            mbc1(static_cast<const Mbc1M<1 * MB, 32 * KB, true>&>(cartridge));
                    } else {
                        if (ram == Ram::NONE)
                            mbc1(static_cast<const Mbc1<1 * MB, 0, true>&>(cartridge));
                        else if (ram == Ram::KB_8)
                            mbc1(static_cast<const Mbc1<1 * MB, 8 * KB, true>&>(cartridge));
                        else if (ram == Ram::KB_32)
                            mbc1(static_cast<const Mbc1<1 * MB, 32 * KB, true>&>(cartridge));
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<2 * MB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<2 * MB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<2 * MB, 32 * KB, true>&>(cartridge));
                }
            } else {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<32 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<32 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<32 * KB, 32 * KB, false>&>(cartridge));
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<64 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<64 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<64 * KB, 32 * KB, false>&>(cartridge));
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<128 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<128 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<128 * KB, 32 * KB, false>&>(cartridge));
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<256 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<256 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<256 * KB, 32 * KB, false>&>(cartridge));
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<512 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<512 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<512 * KB, 32 * KB, false>&>(cartridge));
                } else if (rom == Rom::MB_1) {
                    // All the known MBC1M are 1 MB.
                    if (multicart) {
                        if (ram == Ram::NONE)
                            mbc1(static_cast<const Mbc1M<1 * MB, 0, false>&>(cartridge));
                        else if (ram == Ram::KB_8)
                            mbc1(static_cast<const Mbc1M<1 * MB, 8 * KB, false>&>(cartridge));
                        else if (ram == Ram::KB_32)
                            mbc1(static_cast<const Mbc1M<1 * MB, 32 * KB, false>&>(cartridge));
                    } else {
                        if (ram == Ram::NONE)
                            mbc1(static_cast<const Mbc1M<1 * MB, 0, false>&>(cartridge));
                        else if (ram == Ram::KB_8)
                            mbc1(static_cast<const Mbc1<1 * MB, 8 * KB, false>&>(cartridge));
                        else if (ram == Ram::KB_32)
                            mbc1(static_cast<const Mbc1<1 * MB, 32 * KB, false>&>(cartridge));
                    }
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE)
                        mbc1(static_cast<const Mbc1<2 * MB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc1(static_cast<const Mbc1<2 * MB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc1(static_cast<const Mbc1<2 * MB, 32 * KB, false>&>(cartridge));
                }
            }
        } else if (mbc == Mbc::MBC2 || mbc == Mbc::MBC2_BATTERY) {
            const auto mbc2 = [&b](auto&& cart) {
                b << yellow("Ram Enabled") << "                 :  " << cart.ramEnabled << endl;
                b << yellow("Rom Bank Selector") << "           :  " << hex(cart.romBankSelector) << endl;
            };

            if (mbc == Mbc::MBC2_BATTERY) {
                if (ram == Ram::NONE) {
                    if (rom == Rom::KB_32)
                        mbc2(static_cast<const Mbc2<32 * KB, true>&>(cartridge));
                    else if (rom == Rom::KB_64)
                        mbc2(static_cast<const Mbc2<64 * KB, true>&>(cartridge));
                    else if (rom == Rom::KB_128)
                        mbc2(static_cast<const Mbc2<128 * KB, true>&>(cartridge));
                    else if (rom == Rom::KB_256)
                        mbc2(static_cast<const Mbc2<256 * KB, true>&>(cartridge));
                }
            } else {
                if (ram == Ram::NONE) {
                    if (rom == Rom::KB_32)
                        mbc2(static_cast<const Mbc2<32 * KB, false>&>(cartridge));
                    else if (rom == Rom::KB_64)
                        mbc2(static_cast<const Mbc2<64 * KB, false>&>(cartridge));
                    else if (rom == Rom::KB_128)
                        mbc2(static_cast<const Mbc2<128 * KB, false>&>(cartridge));
                    else if (rom == Rom::KB_256)
                        mbc2(static_cast<const Mbc2<256 * KB, false>&>(cartridge));
                }
            }
        } else if (mbc == Mbc::MBC3_TIMER_BATTERY || mbc == Mbc::MBC3_TIMER_RAM_BATTERY || mbc == Mbc::MBC3 ||
                   mbc == Mbc::MBC3_RAM || mbc == Mbc::MBC3_RAM_BATTERY) {
            const auto mbc3 = [&b, mbc, subheader, width](auto&& cart) {
                b << yellow("Ram/RTC Enabled") << "             :  " << cart.ramEnabled << endl;
                b << yellow("Rom Bank Selector") << "           :  " << hex(cart.romBankSelector) << endl;
                b << yellow("Ram Bank/RTC Reg. Selector") << "  :  " << hex(cart.ramBankSelector_rtcRegisterSelector)
                  << endl;

                if (mbc == Mbc::MBC3_TIMER_BATTERY || mbc == Mbc::MBC3_TIMER_RAM_BATTERY) {
                    b << subheader("rtc", width) << endl;
                    b << "                               " << magenta("Latch") << "  |   " << magenta("Real") << endl;
                    b << yellow("Seconds") << "                     :   "
                      << hex((uint8_t)keep_bits<6>(cart.rtc.latched.seconds)) << "    |    "
                      << hex((uint8_t)keep_bits<6>(cart.rtc.real.seconds)) << endl;
                    b << yellow("Minutes") << "                     :   "
                      << hex((uint8_t)keep_bits<6>(cart.rtc.latched.minutes)) << "    |    "
                      << hex((uint8_t)keep_bits<6>(cart.rtc.real.minutes)) << endl;
                    b << yellow("Hours") << "                       :   "
                      << hex((uint8_t)keep_bits<5>(cart.rtc.latched.hours)) << "    |    "
                      << hex((uint8_t)keep_bits<5>(cart.rtc.real.hours)) << endl;
                    b << yellow("Days Low") << "                    :   " << hex(cart.rtc.latched.days.low)
                      << "    |    " << hex(cart.rtc.real.days.low) << endl;
                    b << yellow("Days High") << "                   :   "
                      << hex((uint8_t)get_bits<7, 6, 0>(cart.rtc.latched.days.high)) << "    |    "
                      << hex((uint8_t)get_bits<7, 6, 0>(cart.rtc.real.days.high)) << endl;
                }
            };

            if (mbc == Mbc::MBC3_TIMER_BATTERY || mbc == Mbc::MBC3_TIMER_RAM_BATTERY) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<32 * KB, 0, true, true>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<32 * KB, 2 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<32 * KB, 8 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<32 * KB, 32 * KB, true, true>&>(cartridge));
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<64 * KB, 0, true, true>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<64 * KB, 2 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<64 * KB, 8 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<64 * KB, 32 * KB, true, true>&>(cartridge));
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<128 * KB, 0, true, true>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<128 * KB, 2 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<128 * KB, 8 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<128 * KB, 32 * KB, true, true>&>(cartridge));
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<256 * KB, 0, true, true>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<256 * KB, 2 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<256 * KB, 8 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<256 * KB, 32 * KB, true, true>&>(cartridge));
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<512 * KB, 0, true, true>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<512 * KB, 2 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<512 * KB, 8 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<512 * KB, 32 * KB, true, true>&>(cartridge));
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<1 * MB, 0, true, true>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<1 * MB, 2 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<1 * MB, 8 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<1 * MB, 32 * KB, true, true>&>(cartridge));
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<2 * MB, 0, true, true>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<2 * MB, 2 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<2 * MB, 8 * KB, true, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<2 * MB, 32 * KB, true, true>&>(cartridge));
                }
            } else if (mbc == Mbc::MBC3 || mbc == Mbc::MBC3_RAM) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<32 * KB, 0, false, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<32 * KB, 2 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<32 * KB, 8 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<32 * KB, 32 * KB, false, false>&>(cartridge));
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<64 * KB, 0, false, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<64 * KB, 2 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<64 * KB, 8 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<64 * KB, 32 * KB, false, false>&>(cartridge));
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<128 * KB, 0, false, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<128 * KB, 2 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<128 * KB, 8 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<128 * KB, 32 * KB, false, false>&>(cartridge));
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<256 * KB, 0, false, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<256 * KB, 2 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<256 * KB, 8 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<256 * KB, 32 * KB, false, false>&>(cartridge));
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<512 * KB, 0, false, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<512 * KB, 2 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<512 * KB, 8 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<512 * KB, 32 * KB, false, false>&>(cartridge));
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<1 * MB, 0, false, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<1 * MB, 2 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<1 * MB, 8 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<1 * MB, 32 * KB, false, false>&>(cartridge));
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<2 * MB, 0, false, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<2 * MB, 2 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<2 * MB, 8 * KB, false, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<2 * MB, 32 * KB, false, false>&>(cartridge));
                }
            } else if (mbc == Mbc::MBC3_RAM_BATTERY) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<32 * KB, 0, true, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<32 * KB, 2 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<32 * KB, 8 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<32 * KB, 32 * KB, true, false>&>(cartridge));
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<64 * KB, 0, true, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<64 * KB, 2 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<64 * KB, 8 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<64 * KB, 32 * KB, true, false>&>(cartridge));
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<128 * KB, 0, true, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<128 * KB, 2 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<128 * KB, 8 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<128 * KB, 32 * KB, true, false>&>(cartridge));
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<256 * KB, 0, true, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<256 * KB, 2 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<256 * KB, 8 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<256 * KB, 32 * KB, true, false>&>(cartridge));
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<512 * KB, 0, true, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<512 * KB, 2 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<512 * KB, 8 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<512 * KB, 32 * KB, true, false>&>(cartridge));
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<1 * MB, 0, true, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<1 * MB, 2 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<1 * MB, 8 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<1 * MB, 32 * KB, true, false>&>(cartridge));
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE)
                        mbc3(static_cast<const Mbc3<2 * MB, 0, true, false>&>(cartridge));
                    else if (ram == Ram::KB_2)
                        mbc3(static_cast<const Mbc3<2 * MB, 2 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc3(static_cast<const Mbc3<2 * MB, 8 * KB, true, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc3(static_cast<const Mbc3<2 * MB, 32 * KB, true, false>&>(cartridge));
                }
            }
        } else if (mbc == Mbc::MBC5 || mbc == Mbc::MBC5_RAM || mbc == Mbc::MBC5_RUMBLE || mbc == Mbc::MBC5_RUMBLE_RAM ||
                   mbc == Mbc::MBC5_RAM_BATTERY || mbc == Mbc::MBC5_RUMBLE_RAM_BATTERY) {
            const auto mbc5 = [&b](auto&& cart) {
                b << yellow("Ram Enabled") << "                  :  " << cart.ramEnabled << endl;
                b << yellow("Rom Bank Selector") << "            :  " << hex(cart.romBankSelector) << endl;
                b << yellow("Ram Bank Selector") << "            :  " << hex(cart.ramBankSelector) << endl;
            };

            if (mbc == Mbc::MBC5_RAM_BATTERY || mbc == Mbc::MBC5_RUMBLE_RAM_BATTERY) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<32 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<32 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<32 * KB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<32 * KB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<32 * KB, 128 * KB, true>&>(cartridge));
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<64 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<64 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<64 * KB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<64 * KB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<64 * KB, 128 * KB, true>&>(cartridge));
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<128 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<128 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<128 * KB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<128 * KB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<128 * KB, 128 * KB, true>&>(cartridge));
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<256 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<256 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<256 * KB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<256 * KB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<256 * KB, 128 * KB, true>&>(cartridge));
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<512 * KB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<512 * KB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<512 * KB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<512 * KB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<512 * KB, 128 * KB, true>&>(cartridge));
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<1 * MB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<1 * MB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<1 * MB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<1 * MB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<1 * MB, 128 * KB, true>&>(cartridge));
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<2 * MB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<2 * MB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<2 * MB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<2 * MB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<2 * MB, 128 * KB, true>&>(cartridge));
                } else if (rom == Rom::MB_4) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<4 * MB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<4 * MB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<4 * MB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<4 * MB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<4 * MB, 128 * KB, true>&>(cartridge));
                } else if (rom == Rom::MB_8) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<8 * MB, 0, true>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<8 * MB, 8 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<8 * MB, 32 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<8 * MB, 64 * KB, true>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<8 * MB, 128 * KB, true>&>(cartridge));
                }
            } else if (mbc == Mbc::MBC5 || mbc == Mbc::MBC5_RAM || mbc == Mbc::MBC5_RUMBLE ||
                       mbc == Mbc::MBC5_RUMBLE_RAM) {
                if (rom == Rom::KB_32) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<32 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<32 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<32 * KB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<32 * KB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<32 * KB, 128 * KB, false>&>(cartridge));
                } else if (rom == Rom::KB_64) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<64 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<64 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<64 * KB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<64 * KB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<64 * KB, 128 * KB, false>&>(cartridge));
                } else if (rom == Rom::KB_128) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<128 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<128 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<128 * KB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<128 * KB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<128 * KB, 128 * KB, false>&>(cartridge));
                } else if (rom == Rom::KB_256) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<256 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<256 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<256 * KB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<256 * KB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<256 * KB, 128 * KB, false>&>(cartridge));
                } else if (rom == Rom::KB_512) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<512 * KB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<512 * KB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<512 * KB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<512 * KB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<512 * KB, 128 * KB, false>&>(cartridge));
                } else if (rom == Rom::MB_1) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<1 * MB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<1 * MB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<1 * MB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<1 * MB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<1 * MB, 128 * KB, false>&>(cartridge));
                } else if (rom == Rom::MB_2) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<2 * MB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<2 * MB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<2 * MB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<2 * MB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<2 * MB, 128 * KB, false>&>(cartridge));
                } else if (rom == Rom::MB_4) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<4 * MB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<4 * MB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<4 * MB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<4 * MB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<4 * MB, 128 * KB, false>&>(cartridge));
                } else if (rom == Rom::MB_8) {
                    if (ram == Ram::NONE)
                        mbc5(static_cast<const Mbc5<8 * MB, 0, false>&>(cartridge));
                    else if (ram == Ram::KB_8)
                        mbc5(static_cast<const Mbc5<8 * MB, 8 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_32)
                        mbc5(static_cast<const Mbc5<8 * MB, 32 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_64)
                        mbc5(static_cast<const Mbc5<8 * MB, 64 * KB, false>&>(cartridge));
                    else if (ram == Ram::KB_128)
                        mbc5(static_cast<const Mbc5<8 * MB, 128 * KB, false>&>(cartridge));
                }
            }
        }

        return b;
    };

    const auto makePpuHeader = [&](uint32_t width) {
        auto b {make_block(width)};
        b << header("PPU", width) << endl;
        return b;
    };

    // PPU
    const auto makePpuBlockPart1 = [&](uint32_t width) {
        auto b {make_block(width)};

        b << subheader("general", width) << endl;

        b << yellow("On") << "               :  " << gb.ppu.on << endl;
        b << yellow("Cycle") << "            :  " << gb.ppu.cycles << endl;
        b << yellow("Dots") << "             :  " << gb.ppu.dots << endl;
        b << yellow("Mode") << "             :  " << [this]() -> Text {
            if (gb.ppu.tickSelector == &Ppu::oamScanEven || gb.ppu.tickSelector == &Ppu::oamScanOdd ||
                gb.ppu.tickSelector == &Ppu::oamScanDone || gb.ppu.tickSelector == &Ppu::oamScanAfterTurnOn)
                return "Oam Scan";
            if (gb.ppu.tickSelector == &Ppu::pixelTransferDummy0 ||
                gb.ppu.tickSelector == &Ppu::pixelTransferDiscard0 ||
                gb.ppu.tickSelector == &Ppu::pixelTransferDiscard0WX0SCX7 ||
                gb.ppu.tickSelector == &Ppu::pixelTransfer0 || gb.ppu.tickSelector == &Ppu::pixelTransfer8) {

                Text t {"Pixel Transfer"};

                std::string blockReason;
                if (gb.ppu.isFetchingSprite)
                    blockReason = "fetching sprite";
                else if (gb.ppu.oamEntries[gb.ppu.LX].isNotEmpty())
                    blockReason = "sprite hit";
                else if (gb.ppu.bgFifo.isEmpty())
                    blockReason = "empty bg fifo";

                if (!blockReason.empty())
                    t += yellow(" [" + blockReason + "]");

                return t;
            }
            if (gb.ppu.tickSelector == &Ppu::hBlank || gb.ppu.tickSelector == &Ppu::hBlank453 ||
                gb.ppu.tickSelector == &Ppu::hBlank454 || gb.ppu.tickSelector == &Ppu::hBlank455 ||
                gb.ppu.tickSelector == &Ppu::hBlankLastLine || gb.ppu.tickSelector == &Ppu::hBlankLastLine454 ||
                gb.ppu.tickSelector == &Ppu::hBlankLastLine455)
                return "Horizontal Blank";
            if (gb.ppu.tickSelector == &Ppu::vBlank || gb.ppu.tickSelector == &Ppu::vBlank454)
                return "Vertical Blank";
            if (gb.ppu.tickSelector == &Ppu::vBlankLastLine || gb.ppu.tickSelector == &Ppu::vBlankLastLine2 ||
                gb.ppu.tickSelector == &Ppu::vBlankLastLine7 || gb.ppu.tickSelector == &Ppu::vBlankLastLine454)
                return "Vertical Blank (Last Line)";

            checkNoEntry();
            return "Unknown";
        }() << endl;

        b << yellow("Last Stat IRQ") << "    :  " << gb.ppu.lastStatIrq << endl;
        b << yellow("LYC_EQ_LY En.") << "    :  " << gb.ppu.enableLycEqLyIrq << endl;

        // LCD
        const auto pixelColor = [this](uint16_t lcdColor) {
            Text colors[4] {
                color<154>("0"),
                color<106>("1"),
                color<64>("2"),
                color<28>("3"),
            };
            for (uint8_t i = 0; i < 4; i++) {
                if (lcdColor == gb.lcd.palette[i])
                    return colors[i];
            }
            return color<0>("[.]"); // allowed because framebuffer could be cleared by the debugger
        };

        b << subheader("lcd", width) << endl;
        b << yellow("X") << "                :  " << gb.lcd.x << endl;
        b << yellow("Y") << "                :  " << gb.lcd.y << endl;
        b << yellow("Last Pixels") << "      :  ";
        for (int32_t i = 0; i < 8; i++) {
            int32_t idx = gb.lcd.cursor - 8 + i;
            if (idx >= 0) {
                b << pixelColor(gb.lcd.pixels[idx]) << " ";
            }
        }
        b << endl;

        // BG Fifo
        b << subheader("bg fifo", width) << endl;

        uint8_t bgPixels[8];
        for (uint8_t i = 0; i < gb.ppu.bgFifo.size(); i++) {
            bgPixels[i] = gb.ppu.bgFifo[i].colorIndex;
        }
        b << yellow("BG Fifo Pixels") << "   :  " << hex(bgPixels, gb.ppu.bgFifo.size()) << endl;

        // OBJ Fifo
        uint8_t objPixels[8];
        uint8_t objAttrs[8];
        uint8_t objNumbers[8];
        uint8_t objXs[8];

        for (uint8_t i = 0; i < gb.ppu.objFifo.size(); i++) {
            const Ppu::ObjPixel& p = gb.ppu.objFifo[i];
            objPixels[i] = p.colorIndex;
            objAttrs[i] = p.attributes;
            objNumbers[i] = p.number;
            objXs[i] = p.x;
        }

        b << subheader("obj fifo", width) << endl;

        b << yellow("OBJ Fifo Pixels") << "  :  " << hex(objPixels, gb.ppu.objFifo.size()) << endl;
        b << yellow("OBJ Fifo Number") << "  :  " << hex(objNumbers, gb.ppu.objFifo.size()) << endl;
        b << yellow("OBJ Fifo Attrs.") << "  :  " << hex(objAttrs, gb.ppu.objFifo.size()) << endl;
        b << yellow("OBJ Fifo X") << "       :  " << hex(objXs, gb.ppu.objFifo.size()) << endl;

        // Window
        b << subheader("window", width) << endl;
        b << yellow("Active for frame") << " :  " << gb.ppu.w.activeForFrame << endl;
        b << yellow("WLY") << "              :  " << (gb.ppu.w.WLY != UINT8_MAX ? gb.ppu.w.WLY : darkgray("None"))
          << endl;
        b << yellow("Active") << "           :  " << gb.ppu.w.active << endl;
        b << yellow("WX Triggers") << "      :  ";
        for (uint8_t i = 0; i < gb.ppu.w.lineTriggers.size(); i++) {
            b << Text {gb.ppu.w.lineTriggers[i]};
            if (i < gb.ppu.w.lineTriggers.size() - 1)
                b << " | ";
        }
        b << endl;

        // OAM Registers
        b << subheader("oam registers", width) << endl;
        b << yellow("OAM A") << "            :  " << hex(gb.ppu.registers.oam.a) << endl;
        b << yellow("OAM B") << "            :  " << hex(gb.ppu.registers.oam.b) << endl;

        // OAM Scanline entries
        const auto& oamEntries = gb.ppu.scanlineOamEntries;
        b << subheader("oam scanline entries", width) << endl;

        const auto oamEntriesInfo = [](const auto& v, uint8_t (*fn)(const Ppu::OamScanEntry&)) {
            Text t {};
            for (uint8_t i = 0; i < v.size(); i++) {
                t += Text {fn(v[i])}.rpad(Text::Length {3}) + (i < v.size() - 1 ? " " : "");
            }
            return t;
        };

        b << yellow("OAM Number") << "       :  " << oamEntriesInfo(oamEntries, [](const Ppu::OamScanEntry& e) {
            return e.number;
        }) << endl;
        b << yellow("OAM X") << "            :  " << oamEntriesInfo(oamEntries, [](const Ppu::OamScanEntry& e) {
            return e.x;
        }) << endl;
        b << yellow("OAM Y") << "            :  " << oamEntriesInfo(oamEntries, [](const Ppu::OamScanEntry& e) {
            return e.y;
        }) << endl;

        if (gb.ppu.LX < array_size(gb.ppu.oamEntries)) {
            const auto& oamEntriesHit = gb.ppu.oamEntries[gb.ppu.LX];
            // OAM Hit
            b << subheader("oam hit", width) << endl;

            b << yellow("OAM Number") << "       :  " << oamEntriesInfo(oamEntriesHit, [](const Ppu::OamScanEntry& e) {
                return e.number;
            }) << endl;
            b << yellow("OAM X") << "            :  " << oamEntriesInfo(oamEntriesHit, [](const Ppu::OamScanEntry& e) {
                return e.x;
            }) << endl;
            b << yellow("OAM Y") << "            :  " << oamEntriesInfo(oamEntriesHit, [](const Ppu::OamScanEntry& e) {
                return e.y;
            }) << endl;
        }

        return b;
    };

    const auto makePpuBlockPart2 = [&](uint32_t width) {
        auto b {make_block(width)};

        // Pixel Transfer
        b << subheader("pixel transfer", width) << endl;

        b << yellow("Fetcher Mode") << "        :  " <<
            [this]() {
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPrefetcherGetTile0)
                    return "BG/WIN Tile0";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgPrefetcherGetTile0)
                    return "BG Tile0";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgPrefetcherGetTile1)
                    return "BG Tile1";
                if (gb.ppu.fetcherTickSelector == &Ppu::winPrefetcherActivating)
                    return "WIN Activating";
                if (gb.ppu.fetcherTickSelector == &Ppu::winPrefetcherGetTile0)
                    return "WIN Tile0";
                if (gb.ppu.fetcherTickSelector == &Ppu::winPrefetcherGetTile1)
                    return "WIN Tile1";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgPixelSliceFetcherGetTileDataLow0)
                    return "BG Low0";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgPixelSliceFetcherGetTileDataLow1)
                    return "BG Low1";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgPixelSliceFetcherGetTileDataHigh0)
                    return "BG High0";
                if (gb.ppu.fetcherTickSelector == &Ppu::winPixelSliceFetcherGetTileDataLow0)
                    return "WIN Low0";
                if (gb.ppu.fetcherTickSelector == &Ppu::winPixelSliceFetcherGetTileDataLow1)
                    return "WIN Low1";
                if (gb.ppu.fetcherTickSelector == &Ppu::winPixelSliceFetcherGetTileDataHigh0)
                    return "WIN High0";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1)
                    return "BG/WIN High1";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherPush)
                    return "BG/WIN Push";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPrefetcherGetTile0)
                    return "OBJ Tile0";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPrefetcherGetTile1)
                    return "OBJ Tile1";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataLow0)
                    return "OBJ Low0";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataLow1)
                    return "OBJ Low1";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataHigh0)
                    return "OBJ High0";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo)
                    return "OBJ High1 & Merge";

                checkNoEntry();
                return "Unknown";
            }()
          << endl;

        b << yellow("LX") << "                  :  " << gb.ppu.LX << endl;
        b << yellow("SCX % 8 Initial") << "     :  " << gb.ppu.pixelTransfer.initialSCX.toDiscard << endl;
        b << yellow("SCX % 8 Discard") << "     :  " << gb.ppu.pixelTransfer.initialSCX.discarded << "/"
          << gb.ppu.pixelTransfer.initialSCX.toDiscard << endl;
        b << yellow("LX 0->8 Discard") << "     :  " << (gb.ppu.LX < 8 ? (Text(gb.ppu.LX) + "/8") : "8/8") << endl;

        // BG/WIN Prefetcher
        b << subheader("bg/win prefetcher", width) << endl;
        b << yellow("LX") << "                  :  " << gb.ppu.bwf.LX << endl;
        b << yellow("Tile Map X") << "          :  " << gb.ppu.bwf.tilemapX << endl;
        b << yellow("Tile Map Y") << "          :  " << gb.ppu.bwf.tilemapY << endl;
        b << yellow("Tile Map Addr.") << "      :  "
          << hex<uint16_t>(Specs::MemoryLayout::VRAM::START + gb.ppu.bwf.vTilemapAddr) << endl;
        b << yellow("Tile Map Tile Addr.") << " :  "
          << hex<uint16_t>(Specs::MemoryLayout::VRAM::START + gb.ppu.bwf.vTilemapTileAddr) << endl;
        b << yellow("Cached Fetch") << "        :  "
          << (gb.ppu.bwf.interruptedFetch.hasData ? hex<uint16_t>(gb.ppu.bwf.interruptedFetch.tileDataHigh << 8 |
                                                                  gb.ppu.bwf.interruptedFetch.tileDataLow)
                                                  : darkgray("None"))
          << endl;

        // OBJ Prefetcher
        b << subheader("obj prefetcher", width) << endl;
        b << yellow("OAM Number") << "          :  " << gb.ppu.of.entry.number << endl;
        b << yellow("Tile Number") << "         :  " << gb.ppu.of.tileNumber << endl;
        b << yellow("OBJ Attributes") << "      :  " << gb.ppu.of.attributes << endl;

        // Pixel Slice Fetcher
        b << subheader("pixel slice fetcher", width) << endl;
        b << yellow("Tile Data Addr.") << "     :  "
          << hex<uint16_t>(Specs::MemoryLayout::VRAM::START + gb.ppu.psf.vTileDataAddress) << endl;
        b << yellow("Tile Data") << "           :  "
          << hex<uint16_t>(gb.ppu.psf.tileDataHigh << 8 | gb.ppu.psf.tileDataLow) << endl;
        b << yellow("Tile Data Ready") << "     :  " << [this]() {
            if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherPush)
                return green("Ready to push");
            if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo)
                return green("Ready to merge");
            return darkgray("Not ready");
        }();
        b << endl;

        return b;
    };

    // Bus
    const auto makeBusBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("BUS", width) << endl;

        const auto busRequest = [](Device::Type dev, uint8_t requests) {
            const uint8_t R = 2 * dev;
            const uint8_t W = 2 * dev + 1;

            if (test_bit(requests, W))
                return red("Write");
            if (test_bit(requests, R))
                return green("Read");
            return darkgray("Idle");
        };

        const auto busRequests = [busRequest](uint8_t requests) {
            return busRequest(Device::Cpu, requests).lpad(Text::Length {5}) + " | " +
                   busRequest(Device::Dma, requests).lpad(Text::Length {5}) + " | " +
                   busRequest(Device::Ppu, requests).lpad(Text::Length {5});
        };

        const auto busAcquired = [](Device::Type dev, uint8_t acquirers) {
            return test_bit(acquirers, dev) ? green("YES") : darkgray("NO");
        };

        const auto busAcquirers = [busAcquired](uint8_t acquirers) {
            return " " + busAcquired(Device::Cpu, acquirers).lpad(Text::Length {3}) + "  |  " +
                   busAcquired(Device::Dma, acquirers).lpad(Text::Length {3}) + "  |  " +
                   busAcquired(Device::Ppu, acquirers).lpad(Text::Length {3});
        };

        const auto busAddress = [busRequest](uint16_t address, uint8_t requests) {
            return requests ? hex(address) : darkgray(hex(address));
        };

        b << "                     " << magenta("CPU") << "  |  " << magenta("DMA") << "  |  " << magenta("PPU")
          << endl;
        b << cyan("EXT Bus") << "      " << endl;
        b << "  " << yellow("Request") << "   :       " << busRequests(gb.extBus.requests) << endl;
        b << "  " << yellow("Address") << "   :  " << busAddress(gb.extBus.address, gb.extBus.requests) << endl;
        b << endl;

        b << cyan("CPU Bus") << "      " << endl;
        b << "  " << yellow("Request") << "   :       " << busRequests(gb.cpuBus.requests) << endl;
        b << "  " << yellow("Address") << "   :  " << busAddress(gb.cpuBus.address, gb.cpuBus.requests) << endl;
        b << endl;

        b << cyan("VRAM Bus") << "      " << endl;
        b << "  " << yellow("Request") << "   :       " << busRequests(gb.vramBus.requests) << endl;
        b << "  " << yellow("Acquired") << "  :       " << busAcquirers(gb.vramBus.acquirers) << endl;
        b << "  " << yellow("Address") << "   :  " << busAddress(gb.vramBus.address, gb.vramBus.requests) << endl;
        b << endl;

        b << cyan("OAM Bus") << "      " << endl;
        b << "  " << yellow("Request") << "   :       " << busRequests(gb.oamBus.requests) << endl;
        b << "  " << yellow("Acquired") << "  :       " << busAcquirers(gb.oamBus.acquirers) << endl;
        b << "  " << yellow("Address") << "   :  " << busAddress(gb.oamBus.address, gb.oamBus.requests) << endl;
        b << endl;

        return b;
    };

    // DMA
    const auto makeDmaBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("DMA", width) << endl;

        if (gb.dma.request.state != Dma::RequestState::None || gb.dma.transferring) {
            b << yellow("DMA Request") << "   :  ";
            b <<
                [this]() {
                    switch (gb.dma.request.state) {
                    case Dma::RequestState::Requested:
                        return green("Requested");
                    case Dma::RequestState::Pending:
                        return green("Pending");
                    case Dma::RequestState::None:
                        return Text {"None"};
                    }
                    checkNoEntry();
                    return Text {};
                }()
              << endl;

            b << yellow("DMA Transfer") << "  :  ";
            b << (gb.dma.transferring ? green("In Progress") : "None");
            b << endl;

            b << yellow("DMA Source") << "    :  " << hex(gb.dma.source) << endl;
            b << yellow("DMA Progress") << "  :  " << hex<uint16_t>(gb.dma.source + gb.dma.cursor) << " => "
              << hex<uint16_t>(Specs::MemoryLayout::OAM::START + gb.dma.cursor) << " [" << gb.dma.cursor << "/"
              << "159]" << endl;
        } else {
            b << yellow("DMA Transfer") << "  :  " << darkgray("None") << endl;
        }

        return b;
    };

    // Timers
    const auto makeTimersBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("TIMERS", width) << endl;
        b << yellow("TMA reload") << " :  " << [this]() -> Text {
            switch (gb.timers.timaState) {
            case TimersIO::TimaReloadState::None:
                return darkgray("None");
            case TimersIO::TimaReloadState::Pending:
                return green("Pending");
            case TimersIO::TimaReloadState::Reload:
                return green("Reloading");
            default:
                return Text {};
            }
        }() << endl;
        b << hr(width) << endl;

        b << red("DIV (16)") << "   :  " << [this]() -> Text {
            uint8_t div = gb.timers.DIV;
            Text t {};
            uint8_t highlightBit = Specs::Timers::TAC_DIV_BITS_SELECTOR[keep_bits<2>(gb.timers.TAC)];
            for (int8_t b = 15; b >= 0; b--) {
                bool high = test_bit(div, b);
                t += ((b == highlightBit) ? yellow(Text {high}) : Text {high});
                if (b == 8)
                    t += " ";
            }
            t += " (" + hex((uint8_t)(div >> 8)) + " " + hex((uint8_t)(div & 0xFF)) + ")";
            return t;
        }() << endl;

        const auto timer = [](uint8_t value) {
            return Text {bin(value)} + "          (" + hex(value) + ")";
        };

        b << red("DIV") << "        :  " << timer(backend.readMemory(Specs::Registers::Timers::DIV)) << endl;
        b << red("TIMA") << "       :  " << timer(backend.readMemory(Specs::Registers::Timers::TIMA)) << endl;
        b << red("TMA") << "        :  " << timer(backend.readMemory(Specs::Registers::Timers::TMA)) << endl;
        b << red("TAC") << "        :  " << timer(backend.readMemory(Specs::Registers::Timers::TAC)) << endl;

        return b;
    };

    // IO
    const auto makeIoBlock = [&](uint32_t width) {
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
                uint8_t value = backend.readMemory(addr);
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
        b << subheader("sound", width) << endl;
        b << ios(Specs::Registers::Sound::REGISTERS, array_size(Specs::Registers::Sound::REGISTERS)) << endl;
        b << subheader("video", width) << endl;
        b << ios(Specs::Registers::Video::REGISTERS, array_size(Specs::Registers::Video::REGISTERS)) << endl;

        return b;
    };

    // Code
    const auto makeCodeBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("CODE", width) << endl;

        if (const auto isrPhase = DebuggerHelpers::getIsrPhase(gb.cpu); isrPhase != std::nullopt) {
            b << yellow("ISR") << "   " << lightgray("M" + std::to_string(*isrPhase + 1) + "/5") << endl;
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

            std::list<DisassembledInstructionEntry> codeView;

            const auto currentInstructionOpt = backend.disassemble(gb.cpu.instruction.address, true);
            check(currentInstructionOpt);

            const auto& currentInstruction = *currentInstructionOpt;

            // Past instructions
            {
                static constexpr uint8_t CODE_VIEW_PAST_INSTRUCTION_COUNT = 6;
                uint8_t n = 0;
                for (int32_t addr = currentInstruction.address - 1; addr >= 0 && n < CODE_VIEW_PAST_INSTRUCTION_COUNT;
                     addr--) {
                    auto instr = backend.getDisassembledInstruction(addr);
                    if (instr) {
                        codeView.emplace_front(static_cast<uint16_t>(addr), *instr,
                                               DisassembledInstructionEntry::Type::Past);
                        n++;
                    }
                }
            }

            // Current instruction
            codeView.emplace_back(currentInstruction.address, currentInstruction.instruction,
                                  DisassembledInstructionEntry::Type::Current);

            // Next instructions
            {
                uint32_t addr = currentInstruction.address + currentInstruction.instruction.size();
                for (uint16_t n = 0; n < autoDisassembleNextInstructions && addr <= 0xFFFF; n++) {
                    const auto knownInstr = backend.getDisassembledInstruction(addr);
                    auto instr = backend.disassemble(addr);
                    if (!instr)
                        break;
                    const bool known = knownInstr && *knownInstr == instr->instruction;
                    addr = instr->address + instr->instruction.size();
                    codeView.emplace_back(instr->address, std::move(instr->instruction),
                                          known ? DisassembledInstructionEntry::Type::Future
                                                : DisassembledInstructionEntry::Type::FutureGuess);
                }
            }

            if (!codeView.empty()) {
                const auto disassembleEntry = [this](const DisassembledInstructionEntry& entry) {
                    Text t {};
                    if (backend.getBreakpoint(entry.address))
                        t += red(Text {Token {DOT, 1}});
                    else
                        t += " ";
                    t += " ";

                    Text instrText {to_string(entry)};

                    if (entry.type == DisassembledInstructionEntry::Type::Current)
                        instrText = green(std::move(instrText));
                    else if (entry.type == DisassembledInstructionEntry::Type::Past)
                        instrText = darkgray(std::move(instrText));
                    else if (entry.type == DisassembledInstructionEntry::Type::FutureGuess)
                        instrText = lightestgray(std::move(instrText));

                    t += instrText;

                    if (entry.type == DisassembledInstructionEntry::Type::Current) {
                        auto [min, max] = instruction_duration(entry.instruction);
                        if (min) {
                            uint8_t microopCounter = gb.cpu.instruction.cycleMicroop;
                            t +=
                                "   " + lightgray("M" + std::to_string(microopCounter + 1) + "/" + std::to_string(min) +
                                                  (max != min ? (std::string(":") + std::to_string(+max)) : ""));
                        }
                    }

                    return t;
                };

                // Fill gaps between non-consecutive instructions with ...
                std::list<DisassembledInstructionEntry>::const_iterator lastEntry;
                for (auto entry = codeView.begin(); entry != codeView.end(); entry++) {
                    if (entry != codeView.begin() &&
                        lastEntry->address + lastEntry->instruction.size() < entry->address)
                        b << "  " << darkgray("...") << endl;
                    b << disassembleEntry(*entry) << endl;
                    lastEntry = entry;
                }
            }
        }

        return b;
    };

    // Call Stack
    const auto makeCallStackBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("CALL STACK", width) << endl;

        const auto& callstack = backend.getCallStack();
        for (const auto& entry : callstack) {
            b << to_string(entry) << endl;
        }

        return b;
    };

    // Breakpoints
    const auto makeBreakpointsBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("BREAKPOINTS", width) << endl;

        const auto breakpoint = [this](const Breakpoint& b) {
            Text t {"(" + std::to_string(b.id) + ") " + hex(b.address)};
            const auto instr = backend.getDisassembledInstruction(b.address);
            if (instr) {
                t += "  :  " + rpad(hex(*instr), 9) + "   " + rpad(instruction_mnemonic(*instr, b.address), 23);
            }
            return lightmagenta(std::move(t));
        };

        for (const auto& br : backend.getBreakpoints())
            b << breakpoint(br) << endl;

        return b;
    };

    // Watchpoints
    const auto makeWatchpointsBlock = [&](uint32_t width) {
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
        for (const auto& w : backend.getWatchpoints()) {
            b << watchpoint(w) << endl;
        }

        return b;
    };

    // Display
    const auto makeDisplayBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("DISPLAY", width) << endl;

        for (const auto& d : displayEntries) {
            b << dumpDisplayEntry(d) << endl;
        }

        return b;
    };

    // Interruption
    const auto makeInterruptionBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        if (std::holds_alternative<ExecutionBreakpointHit>(executionState)) {
            b << header("INTERRUPTION", width) << endl;
            auto hit = std::get<ExecutionBreakpointHit>(executionState).breakpointHit;
            b << "Triggered breakpoint (" << hit.breakpoint.id << ") at address " << hex(hit.breakpoint.address)
              << endl;
        } else if (std::holds_alternative<ExecutionWatchpointHit>(executionState)) {
            b << header("INTERRUPTION", width) << endl;
            const auto hit = std::get<ExecutionWatchpointHit>(executionState).watchpointHit;
            b << "Triggered watchpoint (" << hit.watchpoint.id << ") at address " << hex(hit.address) << endl;
            if (hit.accessType == WatchpointHit::AccessType::Read) {
                b << "Read at address " << hex(hit.address) << ": " << hex(hit.newValue) << endl;
            } else {
                b << "Write at address " << hex(hit.address) << endl;
                b << "Old: " << hex(hit.oldValue) << endl;
                b << "New: " << hex(hit.newValue) << endl;
            }
        } else if (std::holds_alternative<ExecutionInterrupted>(executionState)) {
            b << header("INTERRUPTION", width) << endl;
            b << "User interruption request" << endl;
        }

        return b;
    };

    static constexpr uint32_t COLUMN_1_WIDTH = 48;
    auto c1 {make_vertical_layout()};
    c1->addNode(makeGameboyBlock(COLUMN_1_WIDTH));
    c1->addNode(makeSpaceDivider());
    c1->addNode(makeCpuBlock(COLUMN_1_WIDTH));
    c1->addNode(makeSpaceDivider());
    c1->addNode(makeTimersBlock(COLUMN_1_WIDTH));

    static constexpr uint32_t COLUMN_2_WIDTH = 49;
    auto c2 {make_vertical_layout()};
    c2->addNode(makeCartridgeBlock(COLUMN_2_WIDTH));
    c2->addNode(makeSpaceDivider());
    c2->addNode(makeBusBlock(COLUMN_2_WIDTH));
    c2->addNode(makeSpaceDivider());
    c2->addNode(makeDmaBlock(COLUMN_2_WIDTH));

    static constexpr uint32_t COLUMN_3_PART_1_WIDTH = 52;
    static constexpr uint32_t COLUMN_3_PART_2_WIDTH = 40;
    static constexpr uint32_t COLUMN_3_WIDTH = COLUMN_3_PART_1_WIDTH + COLUMN_3_PART_2_WIDTH + 1;

    auto c3r2 {make_horizontal_layout()};
    c3r2->addNode(makePpuBlockPart1(COLUMN_3_PART_1_WIDTH));
    c3r2->addNode(makeSpaceDivider());
    c3r2->addNode(makePpuBlockPart2(COLUMN_3_PART_2_WIDTH));

    auto c3 {make_vertical_layout()};
    c3->addNode(makePpuHeader(COLUMN_3_WIDTH));
    c3->addNode(std::move(c3r2));

    static constexpr uint32_t COLUMN_4_WIDTH = 66;
    auto c4 {make_vertical_layout()};
    c4->addNode(makeIoBlock(COLUMN_4_WIDTH));

    static constexpr uint32_t FULL_WIDTH = COLUMN_1_WIDTH + COLUMN_2_WIDTH + COLUMN_3_WIDTH + COLUMN_4_WIDTH;

    auto r1 {make_horizontal_layout()};
    r1->addNode(std::move(c1));
    r1->addNode(makeHorizontalLineDivider());
    r1->addNode(std::move(c2));
    r1->addNode(makeHorizontalLineDivider());
    r1->addNode(std::move(c3));
    r1->addNode(makeHorizontalLineDivider());
    r1->addNode(std::move(c4));

    static constexpr uint32_t CODE_WIDTH = 56;
    static constexpr uint32_t CALL_STACK_WIDTH = 36;
    static constexpr uint32_t BREAKPOINTS_WIDTH = 50;
    static constexpr uint32_t WATCHPOINTS_WIDTH = 30;
    static constexpr uint32_t DISPLAY_WIDTH =
        FULL_WIDTH - CODE_WIDTH - CALL_STACK_WIDTH - BREAKPOINTS_WIDTH - WATCHPOINTS_WIDTH;

    auto r2 {make_horizontal_layout()};
    r2->addNode(makeCodeBlock(CODE_WIDTH));
    r2->addNode(makeHorizontalLineDivider());
    r2->addNode(makeCallStackBlock(CALL_STACK_WIDTH));
    r2->addNode(makeHorizontalLineDivider());
    r2->addNode(makeBreakpointsBlock(BREAKPOINTS_WIDTH));
    r2->addNode(makeHorizontalLineDivider());
    r2->addNode(makeWatchpointsBlock(WATCHPOINTS_WIDTH));
    r2->addNode(makeHorizontalLineDivider());
    r2->addNode(makeDisplayBlock(DISPLAY_WIDTH));

    auto root {make_vertical_layout()};
    root->addNode(std::move(r1));
    root->addNode(makeVerticalLineDivider());
    root->addNode(std::move(r2));
    auto interruptionBlock {makeInterruptionBlock(FULL_WIDTH)};
    if (!interruptionBlock->lines.empty()) {
        root->addNode(makeInterruptionBlock(FULL_WIDTH));
    }

    Presenter(std::cout).present(*root);
}

std::string DebuggerFrontend::dumpMemory(uint16_t from, uint32_t n, MemoryOutputFormat fmt,
                                         std::optional<uint8_t> fmtArg, bool raw) const {
    std::string s;

    const auto readMemoryFunc = raw ? &DebuggerBackend::readMemoryRaw : &DebuggerBackend::readMemory;

    if (fmt == MemoryOutputFormat::Hexadecimal) {
        for (uint32_t i = 0; i < n; i++)
            s += hex((backend.*readMemoryFunc)(from + i)) + " ";
    } else if (fmt == MemoryOutputFormat::Binary) {
        for (uint32_t i = 0; i < n; i++)
            s += bin((backend.*readMemoryFunc)(from + i)) + " ";
    } else if (fmt == MemoryOutputFormat::Decimal) {
        for (uint32_t i = 0; i < n; i++)
            s += std::to_string((backend.*readMemoryFunc)(from + i)) + " ";
    } else if (fmt == MemoryOutputFormat::Instruction) {
        backend.disassembleMultiple(from, n, true);
        uint32_t i = 0;
        for (uint32_t address = from; address <= 0xFFFF && i < n;) {
            std::optional<DisassembledInstruction> disas = backend.getDisassembledInstruction(address);
            if (!disas)
                fatal("Failed to disassemble at address " + std::to_string(address));

            s += to_string(DisassembledInstructionReference {static_cast<uint16_t>(address), *disas}) +
                 ((i < n - 1) ? "\n" : "");
            address += disas->size();
            i++;
        }
    } else if (fmt == MemoryOutputFormat::Hexdump) {
        std::vector<uint8_t> data;
        for (uint32_t i = 0; i < n; i++)
            data.push_back((backend.*readMemoryFunc)(from + i));
        uint8_t cols = fmtArg ? *fmtArg : 16;
        s += Hexdump().setBaseAddress(from).showAddresses(true).showAscii(false).setNumColumns(cols).hexdump(
            data.data(), data.size());
    }
    return s;
}

std::string DebuggerFrontend::dumpDisplayEntry(const DebuggerFrontend::DisplayEntry& d) const {
    std::stringstream ss;
    if (std::holds_alternative<DisplayEntry::Examine>(d.expression)) {
        DisplayEntry::Examine dx = std::get<DisplayEntry::Examine>(d.expression);
        ss << d.id << ": "
           << "x" << (dx.raw ? "x" : "") << "/" << dx.length << static_cast<char>(dx.format)
           << (dx.formatArg ? std::to_string(*dx.formatArg) : "") << " " << hex(dx.address) << std::endl;
        ss << dumpMemory(dx.address, dx.length, dx.format, dx.formatArg, dx.raw);
    }
    return ss.str();
}