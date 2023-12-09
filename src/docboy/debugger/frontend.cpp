#include "frontend.h"
#include "backend.h"
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

struct FrontendDisassembleCommand {
    uint16_t n {};
};

struct FrontendDisassembleRangeCommand {
    uint16_t from {};
    uint16_t to {};
};

struct FrontendExamineCommand {
    MemoryOutputFormat format {};
    std::optional<uint8_t> formatArg {};
    uint32_t length {};
    uint16_t address {};
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
    std::optional<uint8_t> level {};
};

struct FrontendDumpDisassembleCommand {};

struct FrontendHelpCommand {};
struct FrontendQuitCommand {};

using FrontendCommand =
    std::variant<FrontendBreakpointCommand, FrontendWatchpointCommand, FrontendDeleteCommand,
                 FrontendAutoDisassembleCommand, FrontendDisassembleCommand, FrontendDisassembleRangeCommand,
                 FrontendExamineCommand, FrontendSearchBytesCommand, FrontendSearchInstructionsCommand,
                 FrontendDisplayCommand, FrontendUndisplayCommand, FrontendTickCommand, FrontendDotCommand,
                 FrontendStepCommand, FrontendMicroStepCommand, FrontendNextCommand, FrontendMicroNextCommand,
                 FrontendFrameCommand, FrontendScanlineCommand, FrontendFrameBackCommand, FrontendContinueCommand,
                 FrontendTraceCommand, FrontendDumpDisassembleCommand, FrontendHelpCommand, FrontendQuitCommand>;

struct FrontendCommandInfo {
    std::regex regex;
    std::string format;
    std::string help;
    std::optional<FrontendCommand> (*parser)(const std::vector<std::string>& groups);
};

enum TraceFlag : uint32_t {
    TraceFlagInstruction = 1,
    TraceFlagRegisters = 1 << 1,
    TraceFlagInterrupts = 1 << 2,
    TraceFlagTimers = 1 << 3,
    TraceFlagStack = 1 << 4,
    TraceFlagDma = 1 << 5,
    TraceFlagPpu = 1 << 6,
    TraceMCycle = 1 << 7,
    TraceTCycle = 1 << 8,
    MaxTrace = (TraceTCycle << 1) - 1,
};

template <typename T>
static T parse_hex(const std::string& s, bool* ok) {
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

static std::vector<uint8_t> parse_hex_str(const std::string& s, bool* ok) {
    const std::string ss = (s.size() % 2 == 0) ? s : ("0" + s);
    std::vector<uint8_t> result {};
    for (uint32_t i = 0; i < ss.size() && (!ok || (*ok)); i += 2) {
        result.push_back(parse_hex<uint8_t>(ss.substr(i, 2), ok));
    }
    return result;
}

static uint16_t address_str_to_addr(const std::string& s, bool* ok) {
    if (std::optional<uint16_t> addr = mnemonic_to_address(s))
        return *addr;
    return parse_hex<uint16_t>(s, ok);
}

static FrontendCommandInfo
    FRONTEND_COMMANDS[] =
        {
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
            {std::regex(R"(d\s*(\d+)?)"), "d [<n>]", "Disassemble next <n> instructions (default = 10)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 FrontendDisassembleCommand cmd {};
                 const std::string& n = groups[0];
                 cmd.n = !n.empty() ? std::stoi(n) : 10;
                 return cmd;
             }},
            {std::regex(R"(d\s+(\w+),(\w+))"), "d <start>,<end>",
             "Disassemble instructions from address <start> to <end>",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 FrontendDisassembleRangeCommand cmd {};
                 bool ok {true};
                 cmd.from = address_str_to_addr(groups[0], &ok);
                 cmd.to = address_str_to_addr(groups[1], &ok);
                 return ok ? std::optional(cmd) : std::nullopt;
             }},
            {std::regex(R"(x(?:/(\d+)?(?:([xhbdi])(\d+)?)?)?\s+(\w+))"), "x[/<length><format>] <addr>",
             "Display memory content at <addr> (<format>: x, h[<cols>], b, d, i)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 FrontendExamineCommand cmd {};
                 bool ok {true};
                 const std::string& length = groups[0];
                 const std::string& format = groups[1];
                 const std::string& formatArg = groups[2];
                 const std::string& address = groups[3];
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
            {std::regex(R"(display(?:/(\d+)?(?:([xhbdi])(\d+)?)?)?\s+(\w+))"), "display[/<length><format>] <addr>",
             "Automatically display memory content content at <addr> (<format>: x, h[<cols>], b, d, i)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 FrontendDisplayCommand cmd {};
                 bool ok {true};
                 const std::string& length = groups[0];
                 const std::string& format = groups[1];
                 const std::string& formatArg = groups[2];
                 const std::string& address = groups[3];
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
                 return FrontendTickCommand {.count = n};
             }},
            {std::regex(R"(\.\s*(\d+)?)"), ". [<count>]", "Continue running for <count> PPU dots (default = 1)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 const std::string& count = groups[0];
                 uint64_t n = count.empty() ? 1 : std::stoi(count);
                 return FrontendDotCommand {.count = n};
             }},
            {std::regex(R"(s\s*(\d+)?)"), "s [<count>]", "Continue running for <count> instructions (default = 1)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 const std::string& count = groups[0];
                 uint64_t n = count.empty() ? 1 : std::stoi(count);
                 return FrontendStepCommand {.count = n};
             }},
            {std::regex(R"(si\s*(\d+)?)"), "si [<count>]",
             "Continue running for <count> micro-operations (default = 1)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 const std::string& count = groups[0];
                 uint64_t n = count.empty() ? 1 : std::stoi(count);
                 return FrontendMicroStepCommand {.count = n};
             }},
            {std::regex(R"(n\s*(\d+)?)"), "n [<count>]",
             "Continue running for <count> instructions at the same stack level (default = 1)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 const std::string& count = groups[0];
                 uint64_t n = count.empty() ? 1 : std::stoi(count);
                 return FrontendNextCommand {.count = n};
             }},
            {std::regex(R"(ni\s*(\d+)?)"), "ni [<count>]",
             "Continue running for <count> micro-operations at the same stack level (default = 1)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 const std::string& count = groups[0];
                 uint64_t n = count.empty() ? 1 : std::stoi(count);
                 return FrontendMicroNextCommand {.count = n};
             }},
            {std::regex(R"(f\s*(\d+)?)"), "f [<count>]", "Continue running for <count> frames (default = 1)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 const std::string& count = groups[0];
                 uint64_t n = count.empty() ? 1 : std::stoi(count);
                 return FrontendFrameCommand {.count = n};
             }},
            {std::regex(R"(fb\s*(\d+)?)"), "fb [<count>]",
             "Step back by <count> frames (default = 1, max = " + std::to_string(600) + ")",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 const std::string& count = groups[0];
                 uint64_t n = count.empty() ? 1 : std::stoi(count);
                 return FrontendFrameBackCommand {.count = n};
             }},
            {std::regex(R"(l\s*(\d+)?)"), "l [<count>]", "Continue running for <count> lines (default = 1)",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 const std::string& count = groups[0];
                 uint64_t n = count.empty() ? 1 : std::stoi(count);
                 return FrontendScanlineCommand {.count = n};
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

static std::optional<FrontendCommand> parse_cmdline(const std::string& s) {
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

static volatile sig_atomic_t sigint_trigger = 0;

static void sigint_handler(int signum) {
    sigint_trigger = 1;
}

static void attach_sigint_handler() {
    struct sigaction sa {};
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
}

static void detach_sigint_handler() {
    sigaction(SIGINT, nullptr, nullptr);
}

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
    numAutoDisassemble = cmd.n;
    return std::nullopt;
}

// Disassemble
template <>
std::optional<Command>
DebuggerFrontend::handleCommand<FrontendDisassembleCommand>(const FrontendDisassembleCommand& cmd) {
    backend.disassemble(gb.cpu.instruction.address, cmd.n);
    reprintUI = true;
    return std::nullopt;
}

// DisassembleRange
template <>
std::optional<Command>
DebuggerFrontend::handleCommand<FrontendDisassembleRangeCommand>(const FrontendDisassembleRangeCommand& cmd) {
    backend.disassembleRange(cmd.from, cmd.to);
    reprintUI = true;
    return std::nullopt;
}

// Examine
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendExamineCommand>(const FrontendExamineCommand& cmd) {
    std::cout << dumpMemory(cmd.address, cmd.length, cmd.format, cmd.formatArg) << std::endl;
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
    std::vector<std::pair<uint16_t, DisassembledInstruction>> disassembled = backend.getDisassembledInstructions();
    for (const auto& [addr, instr] : disassembled) {
        if (mem_find_first(instr.data(), instr.size(), cmd.instruction.data(), cmd.instruction.size())) {
            std::cout << hex(addr) << "    " << instruction_mnemonic(instr, addr) << std::endl;
        }
    }
    return std::nullopt;
}

// Display
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendDisplayCommand>(const FrontendDisplayCommand& cmd) {
    DisplayEntry d = {.id = static_cast<uint32_t>(displayEntries.size()),
                      .format = cmd.format,
                      .formatArg = cmd.formatArg,
                      .length = cmd.length,
                      .expression = DisplayEntry::Examine {.address = cmd.address}};
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
    return TickCommand {.count = cmd.count};
}

// Dot
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendDotCommand>(const FrontendDotCommand& cmd) {
    return DotCommand {.count = cmd.count};
}

// Step
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendStepCommand>(const FrontendStepCommand& cmd) {
    return StepCommand {.count = cmd.count};
}

// MicroStep
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendMicroStepCommand>(const FrontendMicroStepCommand& cmd) {
    return MicroStepCommand {.count = cmd.count};
}

// Next
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendNextCommand>(const FrontendNextCommand& cmd) {
    return NextCommand {.count = cmd.count};
}

// MicroNext
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendMicroNextCommand>(const FrontendMicroNextCommand& cmd) {
    return MicroNextCommand {.count = cmd.count};
}

// Frame
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendFrameCommand>(const FrontendFrameCommand& cmd) {
    return FrameCommand {.count = cmd.count};
}

// FrameBack
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendFrameBackCommand>(const FrontendFrameBackCommand& cmd) {
    return FrameBackCommand {.count = cmd.count};
}

// Scanline
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendScanlineCommand>(const FrontendScanlineCommand& cmd) {
    return ScanlineCommand {.count = cmd.count};
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
        trace = trace ? 0 : MaxTrace;
    std::cout << "Trace: " << +static_cast<uint8_t>(trace) << std::endl;
    return std::nullopt;
}

// DumpDisassemble
template <>
std::optional<Command>
DebuggerFrontend::handleCommand<FrontendDumpDisassembleCommand>(const FrontendDumpDisassembleCommand& cmd) {

    std::vector<std::pair<uint16_t, DisassembledInstruction>> disassembled = backend.getDisassembledInstructions();
    for (uint32_t i = 0; i < disassembled.size(); i++) {
        const auto& [addr, instr] = disassembled[i];
        std::cerr << DisassembleEntry {addr, instr}.toString() << std::endl;
        if (i < disassembled.size() - 1 && addr + instr.size() != disassembled[i + 1].first)
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

    // Speculatively disassemble next instructions
    if (numAutoDisassemble > 0)
        backend.disassemble(gb.cpu.instruction.address, numAutoDisassemble);

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
        } else if (std::holds_alternative<FrontendDisassembleCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendDisassembleCommand>(cmd));
        } else if (std::holds_alternative<FrontendDisassembleRangeCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendDisassembleRangeCommand>(cmd));
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

        const bool mTrace = (trace & TraceMCycle) || (trace & TraceTCycle);
        const bool tTrace = (trace & TraceTCycle);
        const bool isMcycle = tick % 4 == 0;
        const bool isNewInstruction = cpu.instruction.microop.counter == 0;

        if (!(tTrace || (mTrace && isMcycle) || isNewInstruction)) {
            // do not trace
            return;
        }

        std::cerr << std::left << std::setw(12) << "[" + std::to_string(tick) + "] " << (isMcycle ? "* " : "  ");

        if (trace & TraceFlagInstruction) {
            std::string instr;
            if (cpu.halted) {
                instr = "<HALTED>";
            } else {
                if (DebuggerHelpers::isInIsr(cpu)) {
                    instr = "ISR " + hex(cpu.instruction.address);
                } else {
                    backend.disassemble(cpu.instruction.address, 1);
                    auto disas = backend.getDisassembledInstruction(cpu.instruction.address);
                    if (disas)
                        instr = instruction_mnemonic(*disas, cpu.instruction.address);
                    else
                        instr = "unknown";
                }
            }

            std::cerr << std::left << std::setw(28) << instr << "  ";
        }

        if (trace & TraceFlagRegisters) {
            std::cerr << "AF:" << hex(cpu.AF) << " BC:" << hex(cpu.BC) << " DE:" << hex(cpu.DE) << " HL:" << hex(cpu.HL)
                      << " SP:" << hex(cpu.SP) << " PC:" << hex((uint16_t)(cpu.PC)) << "  ";
        }

        if (trace & TraceFlagInterrupts) {
            const InterruptsIO& interrupts = gb.interrupts;
            std::cerr << "IME:"
                      << (cpu.IME == Cpu::ImeState::Enabled ? "1" : (cpu.IME == Cpu::ImeState::Pending ? "!" : "0"))
                      << " IE:" << hex((uint8_t)interrupts.IE) << " IF:" << hex((uint8_t)interrupts.IF) << "  ";
        }

        if (trace & TraceFlagStack) {
            std::cerr << "SP[" << hex((uint16_t)(cpu.SP + 1)) << "]:" << hex(backend.readMemory(cpu.SP + 1)) << " "
                      << "SP[" << hex(cpu.SP) << "]:" << hex(backend.readMemory(cpu.SP)) << " "
                      << "SP[" << hex((uint16_t)(cpu.SP - 1)) << "]:" << hex(backend.readMemory(cpu.SP - 1)) << "  ";
        }

        if (trace & TraceFlagDma) {
            std::cerr << "DMA:" << std::setw(3) << (gb.dma.transferring ? std::to_string(gb.dma.cursor) : "OFF")
                      << "  ";
        }

        if (trace & TraceFlagTimers) {
            std::cerr << "DIc16:" << hex(gb.timers.DIV)
                      << " DIV:" << hex(backend.readMemory(Specs::Registers::Timers::DIV))
                      << " TIMA:" << hex(backend.readMemory(Specs::Registers::Timers::TIMA))
                      << " TMA:" << hex(backend.readMemory(Specs::Registers::Timers::TMA))
                      << " TAC:" << hex(backend.readMemory(Specs::Registers::Timers::TAC)) << "  ";
        }

        if (trace & TraceFlagPpu) {
            const Ppu& ppu = gb.ppu;
            std::cerr << "Mode:" << +keep_bits<2>(gb.video.STAT) << " Dots:" << std::setw(12) << ppu.dots << "  ";
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

    const auto title = [](const Text& text, uint8_t width, const std::string& sep = "—") {
        check(width >= text.size());
        Text t {text.size() > 0 ? (" " + text + " ") : text};

        Token sepToken {sep, true};

        const uint8_t w1 = (width - t.size()) / 2;
        Text pre;
        for (uint8_t i = 0; i < w1; i++)
            pre += sepToken;

        const uint8_t w2 = width - t.size() - w1;
        Text post;
        for (uint8_t i = 0; i < w2; i++)
            post += sepToken;

        return darkgray(std::move(pre)) + t + darkgray(std::move(post));
    };

    const auto header = [title](const char* c, uint8_t width) {
        return title(bold(lightcyan(c)), width);
    };

    const auto subheader = [title](const char* c, uint8_t width) {
        return title(cyan(c), width);
    };

    const auto hr = [title](uint8_t width) {
        return title("", width);
    };

    const auto colorbool = [](bool b) -> Text {
        return b ? b : gray(b);
    };

    const auto makeHorizontalLineDivider = []() {
        return make_divider(Text {} + "  " + Token {"│", 1} + "  ");
    };

    const auto makeVerticalLineDivider = []() {
        return make_divider(Text {Token {"—", 1}});
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

        // Bus data
        b << subheader("bus data", width) << endl;
        b << yellow("Data") << "    :  " << bin(gb.cpu.busData) << " (" << hex(gb.cpu.busData) << ")" << endl;

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
                case Cpu::ImeState::Pending:
                    return yellow("Pending");
                case Cpu::ImeState::Disabled:
                    return darkgray("OFF");
                }
                checkNoEntry();
                return Text {};
            }()
          << endl;

        b << red("IE ") << "     :  " << bin(IE) << endl;
        b << red("IF ") << "     :  " << bin(IF) << endl;

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

    // PPU
    const auto makePpuBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("PPU", width) << endl;

        b << yellow("On") << "               :  " << gb.ppu.on << endl;
        b << yellow("Cycle") << "            :  " << gb.ppu.cycles << endl;
        b << yellow("Mode") << "             :  " << [this]() -> Text {
            if (gb.ppu.tickSelector == &Ppu::oamScanEven || gb.ppu.tickSelector == &Ppu::oamScanOdd ||
                gb.ppu.tickSelector == &Ppu::oamScanDone)
                return "Oam Scan";
            if (gb.ppu.tickSelector == &Ppu::pixelTransferDummy0<false> ||
                gb.ppu.tickSelector == &Ppu::pixelTransferDummy0<true> ||
                gb.ppu.tickSelector == &Ppu::pixelTransferDiscard0 || gb.ppu.tickSelector == &Ppu::pixelTransfer0 ||
                gb.ppu.tickSelector == &Ppu::pixelTransfer8) {

                Text t {"Pixel Transfer"};

                std::string blockReason;
                if (gb.ppu.isFetchingSprite)
                    blockReason = "fetching sprite";
                if (gb.ppu.oamEntries[gb.ppu.LX].isNotEmpty())
                    blockReason = "pending sprite hit";
                if (gb.ppu.bgFifo.isEmpty())
                    blockReason = "empty bg fifo";

                if (!blockReason.empty())
                    t += yellow(" [blocked: " + blockReason + "]");

                return t;
            }
            if (gb.ppu.tickSelector == &Ppu::hBlank || gb.ppu.tickSelector == &Ppu::hBlank454 ||
                gb.ppu.tickSelector == &Ppu::hBlankLastLine || gb.ppu.tickSelector == &Ppu::hBlankLastLine454)
                return "Horizontal Blank";
            if (gb.ppu.tickSelector == &Ppu::vBlank)
                return "Vertical Blank";
            if (gb.ppu.tickSelector == &Ppu::vBlankLastLine)
                return "Vertical Blank (Last Line)";

            checkNoEntry();
            return "Unknown";
        }() << endl;

        b << yellow("Dots") << "             :  " << gb.ppu.dots << endl;
        b << yellow("FetcherMode") << "      :  " <<
            [this]() {
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPrefetcherGetTile0)
                    return "BG/WIN GetTile0";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgPrefetcherGetTile0)
                    return "BG Prefetcher GetTile0";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgPrefetcherGetTile1)
                    return "BG Prefetcher GetTile1";
                if (gb.ppu.fetcherTickSelector == &Ppu::winPrefetcherGetTile0)
                    return "WIN Prefetcher GetTile0";
                if (gb.ppu.fetcherTickSelector == &Ppu::winPrefetcherGetTile1)
                    return "WIN Prefetcher GetTile1";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherGetTileDataLow0)
                    return "BG/WIN PixelSliceFetcher GetTileDataLow0";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherGetTileDataLow1)
                    return "BG/WIN PixelSliceFetcher GetTileDataLow1";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherGetTileDataHigh0)
                    return "BG/WIN PixelSliceFetcher GetTileDataHigh0";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherGetTileDataHigh1)
                    return "BG/WIN PixelSliceFetcher GetTileDataHigr1";
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherPush)
                    return "BG/WIN PixelSliceFetcher Push";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPrefetcherGetTile0)
                    return "OBJ Prefetcher GetTile0";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPrefetcherGetTile1)
                    return "OBJ Prefetcher GetTile1";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataLow0)
                    return "OBJ PixelSliceFetcher GetTileDataLow0";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataLow1)
                    return "OBJ PixelSliceFetcher GetTileDataLow1";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataHigh0)
                    return "OBJ PixelSliceFetcher GetTileDataHigh0";
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo)
                    return "OBJ PixelSliceFetcher GetTileDataHigr1AndMerge";

                checkNoEntry();
                return "Unknown";
            }()
          << endl;

        b << yellow("LX") << "               :  " << gb.ppu.LX << endl;

        // LCD
        const auto colorIndex = [](uint16_t lcdColor) {
            for (uint8_t i = 0; i < 4; i++) {
                if (lcdColor == Lcd::RGB565_PALETTE[i])
                    return hex(i);
            }
            return hex((uint8_t)0); // allowed because framebuffer could be cleared by the debugger
        };

        b << subheader("lcd", width) << endl;
        b << yellow("X") << "                :  " << gb.lcd.x << endl;
        b << yellow("Y") << "                :  " << gb.lcd.x << endl;
        b << yellow("Last Pixels") << "      :  ";
        for (int32_t i = 0; i < 8; i++) {
            int32_t idx = gb.lcd.cursor - 8 + i;
            if (idx >= 0) {
                b << colorIndex(gb.lcd.pixels[idx]) << " ";
            }
        }
        b << endl;

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

        // Pixel Slice Fetcher
        b << subheader("pixel slice fetcher", width) << endl;
        b << yellow("Tile Data Addr.") << "  :  "
          << hex<uint16_t>(Specs::MemoryLayout::VRAM::START + gb.ppu.psf.vTileDataAddress) << endl;
        b << yellow("Tile Data Ready") << "  :  " <<
            [this]() {
                if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherPush)
                    return green("Ready to push");
                if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo)
                    return green("Ready to merge");
                return darkgray("Not ready");
            }()
          << endl;

        // Window
        b << subheader("window", width) << endl;
        b << yellow("WLY") << "              :  " << gb.ppu.w.WLY << endl;

        return b;
    };

    // MMU
    const auto makeMmuBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("MMU", width) << endl;

        const auto busLane = [this](const Mmu::BusLane& lane) {
            Text t {};
            if (lane.state == Mmu::BusLane::State::Read) {
                t += yellow("Request") + "  :  " + green("Read") + "\n";
                t += yellow("Address") + "  :  " + hex(lane.address) + "\n";
                t += yellow("Data") + "     :  " + bin(backend.readMemory(lane.address)) + " (" +
                     hex(backend.readMemory(lane.address)) + ")";
            } else if (lane.state == Mmu::BusLane::State::Write) {
                t += yellow("Request") + "  :  " + red("Write") + "\n";
                t += yellow("Address") + "  :  " + hex(lane.address) + "\n";
                t += yellow("Data") + "     :  " + bin(*lane.data) + " (" + hex(*lane.data) + ")";
            } else {
                t += yellow("Request") + "  :  " + darkgray("None");
            }
            return t;
        };

        b << subheader("cpu request", width) << endl;
        b << busLane(gb.mmu.lanes[MmuDevice::Cpu]) << endl;

        b << subheader("dma request", width) << endl;
        b << busLane(gb.mmu.lanes[MmuDevice::Dma]) << endl;

        return b;
    };

    // Bus
    const auto makeBusBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("BUS", width) << endl;

        const auto acquirers = [&b](const auto& bus) {
            Text t {};
            t += yellow("Acquired By") + "  :  ";
            t += (bus.template isAcquiredBy<AcquirableBusDevice::Dma>() ? Text {"DMA"} : darkgray("DMA")) += " ";
            t += (bus.template isAcquiredBy<AcquirableBusDevice::Ppu>() ? "PPU" : darkgray("PPU"));
            return t;
        };

        b << subheader("ext bus", width) << endl;
        // No info at the moment

        b << subheader("cpu bus", width) << endl;
        // No info at the moment

        b << subheader("vram bus", width) << endl;
        b << acquirers(gb.vramBus) << endl;

        b << subheader("oam bus", width) << endl;
        b << acquirers(gb.oamBus) << endl;

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
            b << yellow("DMA Transfer") << " : " << darkgray("None") << endl;
        }

        // Bus data
        b << subheader("bus data", width) << endl;
        b << yellow("Data") << "         :  " << bin(gb.cpu.busData) << " (" << hex(gb.cpu.busData) << ")" << endl;

        return b;
    };

    // Timers
    const auto makeTimersBlock = [&](uint32_t width) {
        auto b {make_block(width)};

        b << header("TIMERS", width) << endl;

        b << red("DIV (16)") << "  :  " << [this]() -> Text {
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

        b << red("DIV") << "       :  " << timer(backend.readMemory(Specs::Registers::Timers::DIV)) << endl;
        b << red("TIMA") << "      :  " << timer(backend.readMemory(Specs::Registers::Timers::TIMA)) <<
            [this]() {
                switch (gb.timers.timaState) {
                case TimersIO::TimaReloadState::Pending:
                    return yellow("[pending TMA reload]");
                case TimersIO::TimaReloadState::Reload:
                    return yellow("[TMA reload]");
                default:
                    return Text {};
                }
            }()
          << endl;
        b << red("TMA") << "       :  " << timer(backend.readMemory(Specs::Registers::Timers::TMA)) << endl;
        b << red("TAC") << "       :  " << timer(backend.readMemory(Specs::Registers::Timers::TAC)) << endl;

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
            std::list<DisassembleEntry> codeView;

            // Past instructions
            {
                static constexpr uint8_t CODE_VIEW_PAST_INSTRUCTION_COUNT = 6;
                uint8_t n = 0;
                for (int32_t addr = gb.cpu.instruction.address; addr >= 0 && n < CODE_VIEW_PAST_INSTRUCTION_COUNT;
                     addr--) {
                    auto entry = backend.getDisassembledInstruction(addr);
                    if (entry) {
                        DisassembleEntry d = {.address = static_cast<uint16_t>(addr), .disassemble = *entry};
                        codeView.push_front(d);
                        n++;
                    }
                }
            }

            // Next instructions
            {
                static constexpr uint8_t CODE_VIEW_NEXT_INSTRUCTION_COUNT = 12;
                uint8_t n = 0;
                for (int32_t addr = gb.cpu.instruction.address + 1;
                     addr <= 0xFFFF && n < CODE_VIEW_NEXT_INSTRUCTION_COUNT; addr++) {
                    auto entry = backend.getDisassembledInstruction(addr);
                    if (entry) {
                        DisassembleEntry d = {.address = static_cast<uint16_t>(addr), .disassemble = *entry};
                        codeView.push_back(d);
                        n++;
                    }
                }
            }

            if (!codeView.empty()) {
                const auto disassembleEntry = [this](const DisassembleEntry& entry) {
                    uint16_t currentInstructionAddress = gb.cpu.instruction.address;
                    uint8_t currentInstructionMicroop = gb.cpu.instruction.microop.counter;

                    uint16_t instrStart = entry.address;
                    uint16_t instrEnd = instrStart + entry.disassemble.size();
                    bool isCurrentInstruction =
                        instrStart <= currentInstructionAddress && currentInstructionAddress < instrEnd;
                    bool isPastInstruction = currentInstructionAddress >= instrEnd;

                    Text t {};
                    if (backend.getBreakpoint(entry.address))
                        t += red(Text {Token {"•", 1}});
                    else
                        t += " ";
                    t += " ";

                    Text instr {hex(entry.address) + "  :  " + rpad(hex(entry.disassemble), 9) + "   " +
                                rpad(instruction_mnemonic(entry.disassemble, entry.address), 23)};

                    if (isCurrentInstruction)
                        t += green(std::move(instr));
                    else if (isPastInstruction)
                        t += darkgray(std::move(instr));
                    else
                        t += instr;

                    if (isCurrentInstruction) {
                        auto [min, max] = instruction_duration(entry.disassemble);
                        if (min)
                            t += "   " + lightgray("M" + std::to_string(currentInstructionMicroop + 1) + "/" +
                                                   std::to_string(min) +
                                                   (max != min ? (std::string(":") + std::to_string(+max)) : ""));
                    }

                    return t;
                };

                std::list<DisassembleEntry>::const_iterator lastEntry;
                for (auto entry = codeView.begin(); entry != codeView.end(); entry++) {
                    if (entry != codeView.begin() &&
                        lastEntry->address + lastEntry->disassemble.size() < entry->address)
                        b << "  " << darkgray("....") << endl;
                    b << disassembleEntry(*entry) << endl;
                    lastEntry = entry;
                }
            }
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

    static constexpr uint32_t COLUMN_1_WIDTH = 40;
    auto c1 {make_vertical_layout()};
    c1->addNode(makeGameboyBlock(COLUMN_1_WIDTH));
    c1->addNode(makeSpaceDivider());
    c1->addNode(makeCpuBlock(COLUMN_1_WIDTH));

    static constexpr uint32_t COLUMN_2_WIDTH = 40;
    auto c2 {make_vertical_layout()};
    c2->addNode(makeMmuBlock(COLUMN_2_WIDTH));
    c2->addNode(makeSpaceDivider());
    c2->addNode(makeBusBlock(COLUMN_2_WIDTH));
    c2->addNode(makeSpaceDivider());
    c2->addNode(makeDmaBlock(COLUMN_2_WIDTH));
    c2->addNode(makeSpaceDivider());
    c2->addNode(makeTimersBlock(COLUMN_2_WIDTH));

    static constexpr uint32_t COLUMN_3_WIDTH = 66;
    auto c3 {make_vertical_layout()};
    c3->addNode(makeIoBlock(COLUMN_3_WIDTH));

    static constexpr uint32_t COLUMN_4_WIDTH = 70;
    auto c4 {make_vertical_layout()};
    c4->addNode(makePpuBlock(COLUMN_4_WIDTH));

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
    static constexpr uint32_t BREAKPOINTS_WIDTH = 50;
    static constexpr uint32_t WATCHPOINTS_WIDTH = 30;
    static constexpr uint32_t DISPLAY_WIDTH = FULL_WIDTH - CODE_WIDTH - BREAKPOINTS_WIDTH - WATCHPOINTS_WIDTH;

    auto r2 {make_horizontal_layout()};
    r2->addNode(makeCodeBlock(CODE_WIDTH));
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
                                         std::optional<uint8_t> fmtArg) const {
    std::string s;

    if (fmt == MemoryOutputFormat::Hexadecimal) {
        for (uint32_t i = 0; i < n; i++)
            s += hex(backend.readMemory(from + i)) + " ";
    } else if (fmt == MemoryOutputFormat::Binary) {
        for (uint32_t i = 0; i < n; i++)
            s += bin(backend.readMemory(from + i)) + " ";
    } else if (fmt == MemoryOutputFormat::Decimal) {
        for (uint32_t i = 0; i < n; i++)
            s += std::to_string(backend.readMemory(from + i)) + " ";
    } else if (fmt == MemoryOutputFormat::Instruction) {
        backend.disassemble(from, n);
        uint32_t i = 0;
        for (uint32_t address = from; address <= 0xFFFF && i < n;) {
            std::optional<DisassembledInstruction> disas = backend.getDisassembledInstruction(address);
            if (!disas)
                fatal("Failed to disassemble at address" + std::to_string(address));

            DisassembleEntry d = {.address = static_cast<uint16_t>(address), .disassemble = *disas};
            s += d.toString() + ((i < n - 1) ? "\n" : "");
            address += disas->size();
            i++;
        }
    } else if (fmt == MemoryOutputFormat::Hexdump) {
        std::vector<uint8_t> data;
        for (uint32_t i = 0; i < n; i++)
            data.push_back(backend.readMemory(from + i));
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
           << "x/" << d.length << static_cast<char>(d.format) << (d.formatArg ? std::to_string(*d.formatArg) : "")
           << " " << hex(dx.address) << std::endl;
        ss << dumpMemory(dx.address, d.length, d.format, d.formatArg);
    }
    return ss.str();
}

std::string DebuggerFrontend::DisassembleEntry::toString() const {
    std::stringstream ss;
    ss << hex(address) << " : " << std::left << std::setw(9) << hex(disassemble) << "   " << std::left << std::setw(16)
       << instruction_mnemonic(disassemble, address);
    return ss.str();
}