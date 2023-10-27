#include "frontend.h"
#include "backend.h"
#include "docboy/gameboy/gameboy.h"
#include "mnemonics.h"
#include "shared.h"
#include "utils/arrays.h"
#include "utils/formatters.hpp"
#include "utils/hexdump.hpp"
#include "utils/memory.h"
#include "utils/strings.hpp"
#include "utils/termcolor.hpp"
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

struct FrontendShowCommand {
    std::string subject;
};

struct FrontendHideCommand {
    std::string subject;
};

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
                 FrontendDisplayCommand, FrontendUndisplayCommand, FrontendShowCommand, FrontendHideCommand,
                 FrontendTickCommand, FrontendDotCommand, FrontendStepCommand, FrontendMicroStepCommand,
                 FrontendNextCommand, FrontendMicroNextCommand, FrontendFrameCommand, FrontendScanlineCommand,
                 FrontendFrameBackCommand, FrontendContinueCommand, FrontendTraceCommand,
                 FrontendDumpDisassembleCommand, FrontendHelpCommand, FrontendQuitCommand>;

struct FrontendCommandInfo {
    std::regex regex;
    std::string format;
    std::string help;
    std::optional<FrontendCommand> (*parser)(const std::vector<std::string>& groups);
};

enum TraceFlag : uint32_t {
    TraceFlagNoTrace = 0,
    TraceFlagInstruction = 1 << 1,
    TraceFlagRegisters = 1 << 2,
    TraceFlagInterrupts = 1 << 3,
    TraceFlagTimers = 1 << 4,
    TraceFlagPpu = 1 << 5,
    TraceMCycle = 1 << 6,
    TraceTCycle = 1 << 7,
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
            {std::regex(R"(show\s*(\w+))"), "show <section>", "Show a section in the debugger layout",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 FrontendShowCommand cmd {.subject = groups[0]};
                 return cmd;
             }},
            {std::regex(R"(hide\s*(\w+))"), "hide <section>", "Hide a section from the debugger layout",
             [](const std::vector<std::string>& groups) -> std::optional<FrontendCommand> {
                 FrontendHideCommand cmd {.subject = groups[0]};
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
    gb(backend.getGameBoy()) {
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
    printMemory(cmd.address, cmd.length, cmd.format, cmd.formatArg);
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
    printDisplayEntry(d);
    return std::nullopt;
}

// Undisplay
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendUndisplayCommand>(const FrontendUndisplayCommand& cmd) {
    displayEntries.clear();
    return std::nullopt;
}

template <bool B>
static bool setBoolsIfMatch(const std::string& s, std::initializer_list<std::string> targets,
                            std::initializer_list<bool*> bs) {
    for (const auto& target : targets) {
        if (s == target) {
            for (bool* b : bs)
                *b = B;
            return true;
        }
    }
    return false;
}

// Show/Hide
template <bool B>
static bool handleShowHideFrontendCommand(const std::string& subject, DebuggerFrontend::Config& config) {
    if (setBoolsIfMatch<B>(subject, {"cpu"},
                           {
                               &config.sections.breakpoints,
                               &config.sections.watchpoints,
                               &config.sections.cpu,
                               &config.sections.flags,
                               &config.sections.registers,
                               &config.sections.interrupts,
                               &config.sections.timers,
                               &config.sections.io.interrupts,
                               &config.sections.io.timers,
                               &config.sections.code,
                           }))
        return true;

    if (setBoolsIfMatch<B>(subject, {"video", "ppu"},
                           {
                               &config.sections.ppu,
                               &config.sections.io.video,
                           }))
        return true;

    if (setBoolsIfMatch<B>(subject, {"sound", "audio"},
                           {
                               &config.sections.io.sound,
                           }))
        return true;

    if (setBoolsIfMatch<B>(subject, {"joypad", "input"},
                           {
                               &config.sections.io.joypad,
                           }))
        return true;

    if (setBoolsIfMatch<B>(subject, {"serial"},
                           {
                               &config.sections.io.serial,
                           }))
        return true;

    if (setBoolsIfMatch<B>(subject, {"io"},
                           {&config.sections.io.joypad, &config.sections.io.serial, &config.sections.io.timers,
                            &config.sections.io.interrupts, &config.sections.io.sound, &config.sections.io.video}))
        return true;

    if (setBoolsIfMatch<B>(subject, {"timers"}, {&config.sections.timers, &config.sections.io.timers}))
        return true;

    if (setBoolsIfMatch<B>(subject, {"interrupts"}, {&config.sections.interrupts, &config.sections.io.interrupts}))
        return true;

    if (setBoolsIfMatch<B>(subject, {"dma"}, {&config.sections.dma}))
        return true;

    if (setBoolsIfMatch<B>(subject, {"code"}, {&config.sections.code}))
        return true;

    if (setBoolsIfMatch<B>(subject, {"all"},
                           {&config.sections.breakpoints, &config.sections.watchpoints, &config.sections.cpu,
                            &config.sections.ppu, &config.sections.flags, &config.sections.registers,
                            &config.sections.interrupts, &config.sections.timers, &config.sections.dma,
                            &config.sections.io.joypad, &config.sections.io.serial, &config.sections.io.timers,
                            &config.sections.io.interrupts, &config.sections.io.sound, &config.sections.io.video,
                            &config.sections.code}))
        return true;

    return false;
}

// Show
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendShowCommand>(const FrontendShowCommand& cmd) {
    if (!handleShowHideFrontendCommand<true>(cmd.subject, config)) {
        std::cout << "Unknown section '" << cmd.subject << "'" << std::endl;
        return std::nullopt;
    }

    reprintUI = true;
    return std::nullopt;
}

// Hide
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendHideCommand>(const FrontendHideCommand& cmd) {
    if (!handleShowHideFrontendCommand<false>(cmd.subject, config)) {
        std::cout << "Unknown section '" << cmd.subject << "'" << std::endl;
        return std::nullopt;
    }

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
    printHelp();
    return std::nullopt;
}

// Quit
template <>
std::optional<Command> DebuggerFrontend::handleCommand<FrontendQuitCommand>(const FrontendQuitCommand& cmd) {
    return AbortCommand();
}

Command DebuggerFrontend::pullCommand(const ExecutionState& executionState) {
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
        std::cout << termcolor::bold << termcolor::green << ">> " << termcolor::reset;

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
        } else if (std::holds_alternative<FrontendShowCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendShowCommand>(cmd));
        } else if (std::holds_alternative<FrontendHideCommand>(cmd)) {
            commandToSend = handleCommand(std::get<FrontendHideCommand>(cmd));
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
        const bool isNewInstruction = cpu.instruction.microop.counter != 0;

        std::cerr << std::left << std::setw(12) << "[" + std::to_string(tick) + "] ";

        if (!(tTrace || (mTrace && isMcycle) || isNewInstruction)) {
            // do not trace
            return;
        }

        if (trace & TraceFlagInstruction) {
            std::string instr;
            if (cpu.halted) {
                instr = "<HALTED>";
            } else {
                if (!cpu.isInISR()) {
                    backend.disassemble(cpu.instruction.address, 1);
                    auto disas = backend.getDisassembledInstruction(cpu.instruction.address);
                    if (disas)
                        instr = instruction_mnemonic(*disas, cpu.instruction.address);
                    else
                        instr = "unknown";
                } else {
                    instr = "ISR " + hex(cpu.instruction.address);
                }
            }

            std::cerr << std::left << std::setw(28) << instr << ((tTrace && isMcycle) ? "(*)" : "   ") << "  ";
        }

        if (trace & TraceFlagRegisters) {
            std::cerr << "AF:" << hex(cpu.AF) << " BC:" << hex(cpu.BC) << " DE:" << hex(cpu.DE) << " HL:" << hex(cpu.HL)
                      << " SP:" << hex(cpu.SP) << " PC:" << hex((uint16_t)(cpu.PC)) << "  ";
        }

        if (trace & TraceFlagInterrupts) {
            const InterruptsIO& interrupts = gb.interrupts;
            std::cerr << "IE:" << hex((uint8_t)interrupts.IE) << " IF:" << hex((uint8_t)interrupts.IF) << "  ";
        }

        if (trace & TraceFlagTimers) {
            const Timers& timers = gb.timers;
            std::cerr << "DIV:" << hex((uint8_t)timers.DIV) << " TIMA:" << hex((uint8_t)timers.TIMA)
                      << " TMA:" << hex((uint8_t)timers.TMA) << " TAC:" << hex((uint8_t)timers.TAC) << "  ";
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

void DebuggerFrontend::printHelp() {
    auto it = std::max_element(std::begin(FRONTEND_COMMANDS), std::end(FRONTEND_COMMANDS),
                               [](const FrontendCommandInfo& i1, const FrontendCommandInfo& i2) {
                                   return i1.format.length() < i2.format.length();
                               });
    const int longestCommandFormat = static_cast<int>(it->format.length());
    for (const auto& info : FRONTEND_COMMANDS) {
        std::cout << std::left << std::setw(longestCommandFormat) << info.format << " : " << info.help << std::endl;
    }
}

template <uint8_t color>
std::string DebuggerFrontend::title(const std::string& title, const std::string& sep) {
    static constexpr uint32_t WIDTH = 80;
    std::string paddedLabel = title.empty() ? "" : (" " + title + " ");
    uint32_t w2 = (WIDTH - paddedLabel.size()) / 2;
    uint32_t w1 = WIDTH - w2 - paddedLabel.size();
    std::string hr1;
    std::string hr2;
    for (uint32_t i = 0; i < w1; i++)
        hr1 += sep;
    for (uint32_t i = 0; i < w2; i++)
        hr2 += sep;
    std::stringstream ss;
    ss << termcolor::color<240> << hr1 << termcolor::attr<color> << paddedLabel << termcolor::color<240> << hr2
       << termcolor::reset;
    return ss.str();
}

std::string DebuggerFrontend::header(const std::string& t, const std::string& sep) {
    return title<36 /* cyan */>(t, sep);
}

std::string DebuggerFrontend::subheader(const std::string& t, const std::string& sep) {
    return title<32 /* green */>(t, sep);
}

std::string DebuggerFrontend::boolColored(bool value) {
    std::stringstream ss;
    if (value)
        ss << "1";
    else
        ss << termcolor::color<240> << "0" << termcolor::reset;
    return ss.str();
}

std::string DebuggerFrontend::flag(const std::string& name, bool value) {
    std::stringstream ss;
    ss << termcolor::bold << termcolor::red << name << termcolor::reset << " : " << boolColored(value);
    return ss.str();
}

std::string DebuggerFrontend::reg(const std::string& name, uint16_t value) {
    std::stringstream ss;
    ss << termcolor::bold << termcolor::red << name << termcolor::reset << " : " << bin((uint8_t)(value >> 8)) << " "
       << bin((uint8_t)(value & 0xFF)) << " (" << hex((uint8_t)(value >> 8)) << " " << hex((uint8_t)(value & 0xFF))
       << ")";
    return ss.str();
}

std::string DebuggerFrontend::div(const std::string& name, uint16_t value, uint8_t highlightBit, int width) {
    std::stringstream ss;
    ss << termcolor::bold << termcolor::red << std::left << std::setw(width) << name << termcolor::reset << " : ";
    for (int8_t b = 15; b >= 0; b--) {
        if (b == highlightBit)
            ss << termcolor::yellow;
        ss << test_bit(value, b) << termcolor::reset;
        if (b == 8)
            ss << " ";
    }
    ss << " (" << hex((uint8_t)(value >> 8)) << " " << hex((uint8_t)(value & 0xFF)) << ")";
    return ss.str();
}

std::string DebuggerFrontend::timer(const std::string& name, uint8_t value, int width, int valueWidth) {
    std::stringstream ss;
    ss << termcolor::bold << termcolor::red << std::left << std::setw(width) << name << termcolor::reset << " : "
       << std::left << std::setw(valueWidth) << bin(value) << " (" << hex(value) << ")";
    return ss.str();
}

std::string DebuggerFrontend::interrupt(const std::string& name, bool IME, bool IE, bool IF) {
    std::stringstream ss;
    ss << termcolor::bold << termcolor::red << name << "  ";
    if (IME && IE && IF)
        ss << termcolor::cyan << "ON " << termcolor::reset << "  ";
    else
        ss << termcolor::color<240> << "OFF" << termcolor::reset << "  ";

    ss << "IE : " << boolColored(IE) << " | "
       << "IF : " << boolColored(IF) << termcolor::reset;
    return ss.str();
}

std::string DebuggerFrontend::io(uint16_t address, uint8_t value, int width) {
    std::stringstream ss;
    ss << termcolor::color<244> << hex(address) << "   " << std::right << termcolor::bold << termcolor::red << std::left
       << std::setw(width) << address_to_mnemonic(address) << termcolor::reset << " : " << bin(value) << " ("
       << hex(value) << ")";
    return ss.str();
}

std::string DebuggerFrontend::watchpoint(const Watchpoint& w) {
    std::stringstream ss;
    ss << termcolor::yellow << "(" << w.id << ") ";
    if (w.address.from != w.address.to)
        ss << hex(w.address.from) << " - " << hex(w.address.to);
    else
        ss << hex(w.address.from);
    if (w.condition.enabled) {
        if (w.condition.condition.operation == Watchpoint::Condition::Operator::Equal) {
            ss << " == " << hex(w.condition.condition.operand);
        }
    }

    switch (w.type) {
    case Watchpoint::Type::Read:
        ss << " (read)";
        break;
    case Watchpoint::Type::Write:
        ss << " (write)";
        break;
    case Watchpoint::Type::ReadWrite:
        ss << " (read/write)";
        break;
    case Watchpoint::Type::Change:
        ss << " (change)";
        break;
    }

    ss << termcolor::reset;
    return ss.str();
}

std::string DebuggerFrontend::breakpoint(const Breakpoint& b) const {
    std::stringstream ss;
    ss << termcolor::color<13> << "(" << b.id << ") " << hex(b.address);
    auto instruction = backend.getDisassembledInstruction(b.address);
    if (instruction) {
        ss << " : " << std::setw(9) << hex(*instruction) << "   " << std::left << std::setw(16)
           << instruction_mnemonic(*instruction, b.address);
    }
    ss << termcolor::reset;
    return ss.str();
}

void DebuggerFrontend::printUI(const ExecutionState& executionState) const {
    const Cpu& cpu = gb.cpu;

    // CPU
    if (config.sections.cpu) {
        std::cout << header("CPU") << std::endl;
        std::cout << termcolor::yellow << "Halted           :  " << termcolor::reset << gb.cpu.halted << std::endl;
        std::cout << termcolor::yellow << "Cycle            :  " << termcolor::reset << gb.cpu.cycles << std::endl;
    }

    // PPU
    if (config.sections.ppu) {
        auto ppuMode = [this]() -> std::string {
            if (gb.ppu.tickSelector == &Ppu::oamScan0 || gb.ppu.tickSelector == &Ppu::oamScan1)
                return "Oam Scan";
            if (gb.ppu.tickSelector == &Ppu::pixelTransferDummy0<false> ||
                gb.ppu.tickSelector == &Ppu::pixelTransferDummy0<true> ||
                gb.ppu.tickSelector == &Ppu::pixelTransferDiscard0 || gb.ppu.tickSelector == &Ppu::pixelTransfer0 ||
                gb.ppu.tickSelector == &Ppu::pixelTransfer8) {
                std::stringstream ss;
                ss << "Pixel Transfer";

                std::string blockReason;
                if (gb.ppu.isFetchingSprite)
                    blockReason = "fetching sprite";
                if (gb.ppu.oamEntries[gb.ppu.LX].isNotEmpty())
                    blockReason = "pending sprite hit";
                if (gb.ppu.bgFifo.isEmpty())
                    blockReason = "empty bg fifo";

                if (!blockReason.empty())
                    ss << termcolor::yellow << " [blocked: " << blockReason << termcolor::reset << "]";

                return ss.str();
            }
            if (gb.ppu.tickSelector == &Ppu::hBlank)
                return "Horizontal Blank";
            if (gb.ppu.tickSelector == &Ppu::vBlank)
                return "Vertical Blank";

            checkNoEntry();
            return "Unknown";
        };
        auto ppuFetcherMode = [this]() {
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
                return "BG/WIN PixelSliceFetcher GetTileDataHigh1";
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
                return "OBJ PixelSliceFetcher GetTileDataHigh1AndMerge";

            checkNoEntry();
            return "Unknown";
        };

        auto lcdColor = [](uint16_t lcdColor) {
            for (uint8_t i = 0; i < 4; i++) {
                if (lcdColor == Lcd::RGB565_PALETTE[i])
                    return hex(i);
            }
            return hex((uint8_t)0); // allowed because framebuffer could be cleared by the debugger
        };

        std::cout << header("PPU") << std::endl;

        std::cout << subheader("lcd") << std::endl;
        std::cout << termcolor::yellow << "X                :  " << termcolor::reset << +gb.lcd.x << std::endl;
        std::cout << termcolor::yellow << "Y                :  " << termcolor::reset << +gb.lcd.y << std::endl;
        std::cout << termcolor::yellow << "Last Pixels      :  " << termcolor::reset;
        for (int32_t i = 0; i < 8; i++) {
            int32_t idx = gb.lcd.cursor - 8 + i;
            if (idx >= 0)
                std::cout << lcdColor(gb.lcd.pixels[idx]) << " ";
        }
        std::cout << std::endl;

        std::cout << subheader("ppu") << std::endl;
        std::cout << termcolor::yellow << "On               :  " << termcolor::reset << gb.ppu.on << std::endl;
        std::cout << termcolor::yellow << "Cycle            :  " << termcolor::reset << gb.ppu.cycles << std::endl;
        std::cout << termcolor::yellow << "Mode             :  " << termcolor::reset << ppuMode() << std::endl;
        std::cout << termcolor::yellow << "Dots             :  " << termcolor::reset << gb.ppu.dots << std::endl;
        std::cout << termcolor::yellow << "FetcherMode      :  " << termcolor::reset << ppuFetcherMode() << std::endl;
        std::cout << termcolor::yellow << "LX               :  " << termcolor::reset << +gb.ppu.LX << std::endl;

        const auto& oamEntries = gb.ppu.scanlineOamEntries;

        std::cout << subheader("oam scanline entries") << std::endl;
        std::cout << termcolor::yellow << "OAM Number       :  " << termcolor::reset;
        for (uint8_t i = 0; i < oamEntries.size(); i++)
            std::cout << std::setw(3) << +oamEntries[i].number << " ";
        std::cout << std::endl;
        std::cout << termcolor::yellow << "OAM X            :  " << termcolor::reset;
        for (uint8_t i = 0; i < oamEntries.size(); i++)
            std::cout << std::setw(3) << +oamEntries[i].x << " ";
        std::cout << std::endl;
        std::cout << termcolor::yellow << "OAM Y            :  " << termcolor::reset;
        for (uint8_t i = 0; i < oamEntries.size(); i++)
            std::cout << std::setw(3) << +oamEntries[i].y << " ";
        std::cout << std::endl;

        if (gb.ppu.LX < array_size(gb.ppu.oamEntries)) {
            const auto& oamEntriesHit = gb.ppu.oamEntries[gb.ppu.LX];

            std::cout << subheader("oam hit") << std::endl;
            std::cout << termcolor::yellow << "OAM Number       :  " << termcolor::reset;
            for (uint8_t i = 0; i < oamEntriesHit.size(); i++)
                std::cout << std::setw(3) << +(oamEntriesHit[i].number) << " ";
            std::cout << std::endl;
            std::cout << termcolor::yellow << "OAM X            :  " << termcolor::reset;
            for (uint8_t i = 0; i < oamEntriesHit.size(); i++)
                std::cout << std::setw(3) << +(oamEntriesHit[i].x) << " ";
            std::cout << std::endl;
            std::cout << termcolor::yellow << "OAM Y            :  " << termcolor::reset;
            for (uint8_t i = 0; i < oamEntriesHit.size(); i++)
                std::cout << std::setw(3) << +(oamEntriesHit[i].y) << " ";
            std::cout << std::endl;
        }

        uint8_t bgPixels[8];
        for (uint8_t i = 0; i < gb.ppu.bgFifo.size(); i++) {
            bgPixels[i] = gb.ppu.bgFifo[i].colorIndex;
        }
        std::cout << subheader("bg fifo") << std::endl;
        std::cout << termcolor::yellow << "BG Fifo Pixels   :  " << termcolor::reset
                  << hex(bgPixels, gb.ppu.bgFifo.size()) << std::endl;

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
        std::cout << subheader("obj fifo") << std::endl;
        std::cout << termcolor::yellow << "OBJ Fifo Pixels  :  " << termcolor::reset
                  << hex(objPixels, gb.ppu.objFifo.size()) << std::endl;
        std::cout << termcolor::yellow << "OBJ Fifo Number  :  " << termcolor::reset
                  << hex(objNumbers, gb.ppu.objFifo.size()) << std::endl;
        std::cout << termcolor::yellow << "OBJ Fifo Attribs :  " << termcolor::reset
                  << hex(objAttrs, gb.ppu.objFifo.size()) << std::endl;
        std::cout << termcolor::yellow << "OBJ Fifo X       :  " << termcolor::reset
                  << hex(objXs, gb.ppu.objFifo.size()) << std::endl;

        std::cout << subheader("pixel slice fetcher") << std::endl;
        std::cout << termcolor::yellow << "Tile Data Addr.  : " << termcolor::reset
                  << hex<uint16_t>(Specs::MemoryLayout::VRAM::START + gb.ppu.psf.vTileDataAddress) << std::endl;
        if (gb.ppu.fetcherTickSelector == &Ppu::bgwinPixelSliceFetcherPush)
            std::cout << termcolor::yellow << "Tile Data Ready  : " << termcolor::green << "Ready to push"
                      << termcolor::reset << std::endl;
        else if (gb.ppu.fetcherTickSelector == &Ppu::objPixelSliceFetcherGetTileDataHigh1AndMergeWithObjFifo)
            std::cout << termcolor::yellow << "Tile Data Ready  : " << termcolor::green << "Ready to merge"
                      << termcolor::reset << std::endl;
        else
            std::cout << termcolor::yellow << "Tile Data Ready  : " << termcolor::color<240> << "Not ready"
                      << termcolor::reset << std::endl;

        std::cout << subheader("win") << std::endl;
        std::cout << termcolor::yellow << "WLY              :  " << termcolor::reset << +gb.ppu.w.WLY << std::endl;

        std::cout << subheader("timings") << std::endl;
        std::cout << termcolor::yellow << "OAM Scan         :  " << termcolor::reset << gb.ppu.timings.oamScan
                  << std::endl;
        std::cout << termcolor::yellow << "Pixel Transfer   :  " << termcolor::reset << gb.ppu.timings.pixelTransfer
                  << std::endl;
        std::cout << termcolor::yellow << "HBlank           :  " << termcolor::reset << gb.ppu.timings.hBlank
                  << std::endl;
    }

    // DMA
    if (config.sections.dma) {
        std::cout << header("DMA") << std::endl;
        if (gb.dma.request.state != 0 || gb.dma.transferring) {
            std::cout << termcolor::yellow << "DMA Request      : ";
            if (gb.dma.request.state == 1) {
                std::cout << termcolor::green << "Requested" << termcolor::reset;
            }
            if (gb.dma.request.state == 2) {
                std::cout << termcolor::green << "Pending" << termcolor::reset;
            } else if (gb.dma.request.state == 0) {
                std::cout << "None";
            }
            std::cout << std::endl;

            std::cout << termcolor::yellow << "DMA Transfer     : ";
            if (gb.dma.transferring) {
                std::cout << termcolor::green << "In Progress" << termcolor::reset;
            } else {
                std::cout << "None";
            }
            std::cout << std::endl;

            std::cout << termcolor::yellow << "DMA Source       : " << termcolor::reset << hex(gb.dma.source)
                      << std::endl;
            std::cout << termcolor::yellow << "DMA Progress     : " << termcolor::reset
                      << hex(gb.dma.source + gb.dma.cursor) << " => "
                      << hex(Specs::MemoryLayout::OAM::START + gb.dma.cursor) << " [" << +gb.dma.cursor << "/"
                      << "159]" << std::endl;
        } else {
            std::cout << termcolor::yellow << "DMA Transfer     : " << termcolor::color<240> << "None"
                      << termcolor::reset << std::endl;
        }
    }

    // Flags
    if (config.sections.flags) {
        std::cout << header("flags") << std::endl;
        std::cout << flag("Z", get_bit<Specs::Bits::Flags::Z>(cpu.AF)) << "  |  "
                  << flag("N", get_bit<Specs::Bits::Flags::N>(cpu.AF)) << "  |  "
                  << flag("H", get_bit<Specs::Bits::Flags::H>(cpu.AF)) << "  |  "
                  << flag("C", get_bit<Specs::Bits::Flags::C>(cpu.AF)) << std::endl;
    }

    // Registers
    if (config.sections.registers) {
        std::cout << header("registers") << std::endl;
        std::cout << reg("AF", cpu.AF) << std::endl;
        std::cout << reg("BC", cpu.BC) << std::endl;
        std::cout << reg("DE", cpu.DE) << std::endl;
        std::cout << reg("HL", cpu.HL) << std::endl;
        std::cout << reg("PC", cpu.PC) << std::endl;
        std::cout << reg("SP", cpu.SP) << std::endl;
    }

    // Interrupts
    if (config.sections.interrupts) {
        uint8_t IE = gb.interrupts.IE;
        uint8_t IF = gb.interrupts.IF;

        std::cout << header("interrupts") << std::endl;
        std::cout << termcolor::bold << termcolor::red << "IME" << termcolor::reset << " : " << boolColored(cpu.IME)
                  << std::endl;
        std::cout << termcolor::bold << termcolor::red << "IE" << termcolor::reset << "  : " << bin(IE) << std::endl;
        std::cout << termcolor::bold << termcolor::red << "IF" << termcolor::reset << "  : " << bin(IF) << std::endl;
        std::cout << interrupt("VBLANK", cpu.IME, get_bit<Specs::Bits::Interrupts::VBLANK>(IE),
                               get_bit<Specs::Bits::Interrupts::VBLANK>(IF))
                  << std::endl;
        std::cout << interrupt("STAT  ", cpu.IME, get_bit<Specs::Bits::Interrupts::STAT>(IE),
                               get_bit<Specs::Bits::Interrupts::STAT>(IF))
                  << std::endl;
        std::cout << interrupt("TIMER ", cpu.IME, get_bit<Specs::Bits::Interrupts::TIMER>(IE),
                               get_bit<Specs::Bits::Interrupts::TIMER>(IF))
                  << std::endl;
        std::cout << interrupt("JOYPAD", cpu.IME, get_bit<Specs::Bits::Interrupts::JOYPAD>(IE),
                               get_bit<Specs::Bits::Interrupts::JOYPAD>(IF))
                  << std::endl;
        std::cout << interrupt("SERIAL", cpu.IME, get_bit<Specs::Bits::Interrupts::SERIAL>(IE),
                               get_bit<Specs::Bits::Interrupts::SERIAL>(IF))
                  << std::endl;
    }

    // Timers
    if (config.sections.timers) {
        std::cout << header("timers") << std::endl;
        std::cout << div("DIV (16)", gb.timers.DIV, Specs::Timers::TAC_DIV_BITS_SELECTOR[keep_bits<2>(gb.timers.TAC)])
                  << std::endl;
        std::cout << timer("DIV", backend.readMemory(Specs::Registers::Timers::DIV)) << std::endl;
        std::cout << timer("TIMA", backend.readMemory(Specs::Registers::Timers::TIMA));
        if (gb.timers.timaState)
            std::cout << termcolor::yellow << "    [pending TMA reload (" << gb.timers.timaState << ")]" << std::endl;
        else
            std::cout << std::endl;
        std::cout << timer("TMA", backend.readMemory(Specs::Registers::Timers::TMA)) << std::endl;
        std::cout << timer("TAC", backend.readMemory(Specs::Registers::Timers::TAC)) << std::endl;
    }

    // IO
    if (config.sections.io.joypad || config.sections.io.serial || config.sections.io.timers ||
        config.sections.io.interrupts || config.sections.io.sound || config.sections.io.video) {
        std::cout << header("IO") << std::endl;
        if (config.sections.io.joypad) {
            std::cout << subheader("joypad") << std::endl;
            printIO(Specs::Registers::Joypad::REGISTERS, array_size(Specs::Registers::Joypad::REGISTERS));
        }
        if (config.sections.io.serial) {
            std::cout << subheader("serial") << std::endl;
            printIO(Specs::Registers::Serial::REGISTERS, array_size(Specs::Registers::Serial::REGISTERS));
        }
        if (config.sections.io.timers) {
            std::cout << subheader("timers") << std::endl;
            printIO(Specs::Registers::Timers::REGISTERS, array_size(Specs::Registers::Timers::REGISTERS));
        }
        if (config.sections.io.interrupts) {
            std::cout << subheader("interrupts") << std::endl;
            printIO(Specs::Registers::Interrupts::REGISTERS, array_size(Specs::Registers::Interrupts::REGISTERS));
        }
        if (config.sections.io.sound) {
            std::cout << subheader("sound") << std::endl;
            printIO(Specs::Registers::Sound::REGISTERS, array_size(Specs::Registers::Sound::REGISTERS));
        }
        if (config.sections.io.video) {
            std::cout << subheader("video") << std::endl;
            printIO(Specs::Registers::Video::REGISTERS, array_size(Specs::Registers::Video::REGISTERS));
        }
    }

    // Code (disassemble)
    if (config.sections.code) {
        if (cpu.isInISR()) {
            std::cout << subheader("code") << std::endl;
            std::cout << termcolor::yellow << "ISR";
            std::cout << termcolor::reset << termcolor::color<245> << "   M" << (cpu.instruction.microop.counter + 1)
                      << "/" << 5 << termcolor::reset << std::endl;
        } else {
            std::list<DisassembleEntry> codeView;
            uint8_t n = 0;
            for (int32_t addr = cpu.instruction.address; addr >= 0 && n < 6; addr--) {
                auto entry = backend.getDisassembledInstruction(addr);
                if (entry) {
                    DisassembleEntry d = {.address = static_cast<uint16_t>(addr), .disassemble = *entry};
                    codeView.push_front(d);
                    n++;
                }
            }
            n = 0;
            for (int32_t addr = cpu.instruction.address + 1; addr <= 0xFFFF && n < 8; addr++) {
                auto entry = backend.getDisassembledInstruction(addr);
                if (entry) {
                    DisassembleEntry d = {.address = static_cast<uint16_t>(addr), .disassemble = *entry};
                    codeView.push_back(d);
                    n++;
                }
            }

            if (!codeView.empty()) {
                std::cout << header("code") << std::endl;
                std::list<DisassembleEntry>::const_iterator lastEntry;
                for (auto entry = codeView.begin(); entry != codeView.end(); entry++) {
                    if (entry != codeView.begin()) {
                        if (lastEntry->address + lastEntry->disassemble.size() < entry->address)
                            std::cout << termcolor::color<240> << "  ...." << termcolor::reset << std::endl;
                    }
                    std::cout << disassembleEntry(*entry) << std::endl;
                    lastEntry = entry;
                }
            }
        }
    }

    // Breakpoints
    if (config.sections.breakpoints) {
        const auto& breakpoints = backend.getBreakpoints();
        if (!breakpoints.empty()) {
            std::cout << header("breakpoints") << std::endl;
            for (const auto& b : breakpoints)
                std::cout << breakpoint(b) << std::endl;
        }
    }

    // Watchpoints
    if (config.sections.watchpoints) {
        const auto& watchpoints = backend.getWatchpoints();
        if (!watchpoints.empty()) {
            std::cout << header("watchpoints") << std::endl;
            for (const auto& w : watchpoints)
                std::cout << watchpoint(w) << std::endl;
        }
    }

    // Display
    if (!displayEntries.empty()) {
        std::cout << header("display") << std::endl;
        for (const auto& d : displayEntries) {
            printDisplayEntry(d);
        }
    }

    // Stop reason
    if (std::holds_alternative<ExecutionBreakpointHit>(executionState)) {
        std::cout << header("interruption") << std::endl;
        auto hit = std::get<ExecutionBreakpointHit>(executionState).breakpointHit;
        std::cout << "Triggered breakpoint [" << hit.breakpoint.id << "] at address " << hex(hit.breakpoint.address)
                  << std::endl;
    } else if (std::holds_alternative<ExecutionWatchpointHit>(executionState)) {
        std::cout << header("interruption") << std::endl;
        auto hit = std::get<ExecutionWatchpointHit>(executionState).watchpointHit;
        std::cout << "Triggered watchpoint [" << hit.watchpoint.id << "] at address " << hex(hit.address) << std::endl;
        if (hit.accessType == WatchpointHit::AccessType::Read) {
            std::cout << "Read at address " << hex(hit.address) << ": " << hex(hit.newValue) << std::endl;
        } else {
            std::cout << "Write at address " << hex(hit.address) << std::endl;
            std::cout << "Old: " << hex(hit.oldValue) << std::endl;
            std::cout << "New: " << hex(hit.newValue) << std::endl;
        }
    } else if (std::holds_alternative<ExecutionInterrupted>(executionState)) {
        std::cout << header("interruption") << std::endl;
        std::cout << "User interruption requested" << std::endl;
    }
}

void DebuggerFrontend::printMemory(uint16_t from, uint32_t n, MemoryOutputFormat fmt,
                                   std::optional<uint8_t> fmtArg) const {
    if (fmt == MemoryOutputFormat::Hexadecimal) {
        for (uint32_t i = 0; i < n; i++)
            std::cout << hex(backend.readMemory(from + i)) << " ";
        std::cout << std::endl;
    } else if (fmt == MemoryOutputFormat::Binary) {
        for (uint32_t i = 0; i < n; i++)
            std::cout << bin(backend.readMemory(from + i)) << " ";
        std::cout << std::endl;
    } else if (fmt == MemoryOutputFormat::Decimal) {
        for (uint32_t i = 0; i < n; i++)
            std::cout << +backend.readMemory(from + i) << " ";
        std::cout << std::endl;
    } else if (fmt == MemoryOutputFormat::Instruction) {
        printInstructions(from, n);
    } else if (fmt == MemoryOutputFormat::Hexdump) {
        std::vector<uint8_t> data;
        for (uint32_t i = 0; i < n; i++)
            data.push_back(backend.readMemory(from + i));
        uint8_t cols = fmtArg ? *fmtArg : 16;
        std::string hexdump =
            Hexdump().setBaseAddress(from).showAddresses(true).showAscii(false).setNumColumns(cols).hexdump(
                data.data(), data.size());
        std::cout << hexdump << std::endl;
    }
}

void DebuggerFrontend::printInstructions(uint16_t from, uint32_t n) const {
    backend.disassemble(from, n);
    uint32_t i = 0;
    for (uint32_t address = from; address <= 0xFFFF && i < n;) {
        std::optional<DisassembledInstruction> disas = backend.getDisassembledInstruction(address);
        if (!disas)
            fatal("Failed to disassemble at address" + std::to_string(address));

        DisassembleEntry d = {.address = static_cast<uint16_t>(address), .disassemble = *disas};
        std::cout << d.toString() << std::endl;
        address += disas->size();
        i++;
    }
}

void DebuggerFrontend::printDisplayEntry(const DebuggerFrontend::DisplayEntry& d) const {
    if (std::holds_alternative<DisplayEntry::Examine>(d.expression)) {
        DisplayEntry::Examine dx = std::get<DisplayEntry::Examine>(d.expression);
        std::cout << d.id << ": "
                  << "x/" << d.length << static_cast<char>(d.format)
                  << (d.formatArg ? std::to_string(*d.formatArg) : "") << " " << hex(dx.address) << std::endl;
        printMemory(dx.address, d.length, d.format, d.formatArg);
    }
}

void DebuggerFrontend::printIO(const uint16_t* addresses, uint32_t length, uint32_t columns) const {
    uint32_t i;
    for (i = 0; i < length;) {
        uint16_t addr = addresses[i];
        uint8_t value = backend.readMemory(addr);
        std::cout << io(addr, value);
        i++;
        if (i % columns == 0)
            std::cout << std::endl;
        else
            std::cout << "    ";
    }
    if (i % columns != 0)
        std::cout << std::endl;
}

std::string DebuggerFrontend::disassembleEntry(const DebuggerFrontend::DisassembleEntry& entry) const {
    uint16_t currentInstructionAddress = gb.cpu.instruction.address;
    uint8_t currentInstructionMicroop = gb.cpu.instruction.microop.counter;

    uint16_t instrStart = entry.address;
    uint16_t instrEnd = instrStart + entry.disassemble.size();
    bool isCurrentInstruction = instrStart <= currentInstructionAddress && currentInstructionAddress < instrEnd;
    bool isPastInstruction = currentInstructionAddress >= instrEnd;

    std::stringstream ss;

    if (backend.getBreakpoint(entry.address))
        ss << termcolor::bold << termcolor::red << "• " << termcolor::reset;
    else
        ss << "  ";

    if (isCurrentInstruction)
        ss << termcolor::bold << termcolor::green;
    else if (isPastInstruction)
        ss << termcolor::color<240>;

    ss << hex(entry.address) << " : " << std::left << std::setw(9) << hex(entry.disassemble) << "   " << std::left
       << std::setw(16) << instruction_mnemonic(entry.disassemble, entry.address);

    if (isCurrentInstruction) {
        auto [min, max] = instruction_duration(entry.disassemble);
        if (min) {
            ss << termcolor::reset << termcolor::color<245> << "   M" << (currentInstructionMicroop + 1) << "/" << +min
               << (max != min ? (std::string(":") + std::to_string(+max)) : "") << termcolor::reset;
        }
    }
    return ss.str();
}

std::string DebuggerFrontend::DisassembleEntry::toString() const {
    std::stringstream ss;
    ss << hex(address) << " : " << std::left << std::setw(9) << hex(disassemble) << "   " << std::left << std::setw(16)
       << instruction_mnemonic(disassemble, address);
    return ss.str();
}