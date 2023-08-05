#include "frontendcli.h"
#include "backend.h"
#include "core/helpers.h"
#include "utils/hexdump.h"
#include "utils/strutils.h"
#include "utils/termcolor.h"
#include <csignal>
#include <iostream>
#include <list>
#include <regex>
#include <sstream>
#include <variant>
#include <vector>

struct CommandBreakpoint {
    uint16_t address;
};

struct CommandWatchpoint {
    Debugger::Watchpoint::Type type;
    uint16_t length;
    struct {
        uint16_t from;
        uint16_t to;
    } address;
    struct {
        bool enabled;
        Debugger::Watchpoint::Condition condition;
    } condition;
};

struct CommandDelete {
    std::optional<uint16_t> num;
};

struct CommandDisassemble {
    uint16_t n;
};

struct CommandDisassembleRange {
    uint16_t from;
    uint16_t to;
};

struct CommandExamine {
    char format;
    std::optional<uint8_t> formatArg;
    size_t length;
    uint16_t address;
};

struct CommandDisplay {
    char format;
    std::optional<uint8_t> formatArg;
    size_t length;
    uint16_t address;
};

struct CommandUndisplay {};

struct CommandDot {
    uint64_t count;
};

struct CommandStep {
    uint64_t count;
};

struct CommandMicroStep {
    uint64_t count;
};

struct CommandNext {
    uint64_t count;
};

struct CommandMicroNext {
    uint64_t count;
};

struct CommandFrame {
    uint64_t count;
};

struct CommandContinue {};

struct CommandTrace {
    std::optional<bool> enabled;
};

struct CommandHelp {};
struct CommandQuit {};

typedef std::variant<CommandBreakpoint, CommandWatchpoint, CommandDelete, CommandDisassemble, CommandDisassembleRange,
                     CommandExamine, CommandDisplay, CommandUndisplay, CommandDot, CommandStep, CommandMicroStep,
                     CommandNext, CommandMicroNext, CommandFrame, CommandContinue, CommandTrace, CommandHelp,
                     CommandQuit>
    Command;

struct CommandInfo {
    std::regex regex;
    std::string format;
    std::string help;
    Command (*parser)(const std::vector<std::string>& groups);
};

static CommandInfo COMMANDS[] = {
    {std::regex(R"(b\s+([0-9a-fA-F]{1,4}))"), "b <addr>", "Set breakpoint at <addr>",
     [](const std::vector<std::string>& groups) -> Command {
         CommandBreakpoint cmd {};
         cmd.address = std::stoi(groups[0], nullptr, 16);
         return cmd;
     }},
    {std::regex(R"(w(?:/([ra]))?\s+([0-9a-fA-F]{1,4}),([0-9a-fA-F]{1,4})\s*(.*)?)"), "w[/r|a] <start>,<end> [<cond>]",
     "Set watchpoint from <start> to <end>",
     [](const std::vector<std::string>& groups) -> Command {
         CommandWatchpoint cmd {};
         const std::string& access = groups[0];
         const std::string& from = groups[1];
         const std::string& to = groups[2];
         const std::string& condition = groups[3];

         if (access == "r")
             cmd.type = Debugger::Watchpoint::Type::Read;
         else if (access == "a")
             cmd.type = Debugger::Watchpoint::Type::ReadWrite;
         else
             cmd.type = Debugger::Watchpoint::Type::Change;

         cmd.address.from = std::stoi(from, nullptr, 16);
         cmd.address.to = std::stoi(to, nullptr, 16);

         std::vector<std::string> tokens;
         split(condition, std::back_inserter(tokens));
         if (tokens.size() >= 2) {
             const auto& operation = tokens[0];
             const auto& operand = tokens[1];
             if (operation == "==") {
                 cmd.condition.enabled = true;
                 cmd.condition.condition.operation = Debugger::Watchpoint::Condition::Operator::Equal;
                 cmd.condition.condition.operand = std::stoi(operand, nullptr, 16);
             }
         }

         return cmd;
     }},
    {std::regex(R"(w(?:/([ra]))?\s+([0-9a-fA-F]{1,4})\s*(.*)?)"), "w[/r|a] <addr> [<cond>]", "Set watchpoint at <addr>",
     [](const std::vector<std::string>& groups) -> Command {
         CommandWatchpoint cmd {};
         const std::string& access = groups[0];
         const std::string& from = groups[1];
         const std::string& condition = groups[2];

         if (access == "r")
             cmd.type = Debugger::Watchpoint::Type::Read;
         else if (access == "a")
             cmd.type = Debugger::Watchpoint::Type::ReadWrite;
         else
             cmd.type = Debugger::Watchpoint::Type::Change;

         cmd.address.from = cmd.address.to = std::stoi(from, nullptr, 16);

         std::vector<std::string> tokens;
         split(condition, std::back_inserter(tokens));
         if (tokens.size() >= 2) {
             const auto& operation = tokens[0];
             const auto& operand = tokens[1];
             if (operation == "==") {
                 cmd.condition.enabled = true;
                 cmd.condition.condition.operation = Debugger::Watchpoint::Condition::Operator::Equal;
                 cmd.condition.condition.operand = std::stoi(operand, nullptr, 16);
             }
         }

         return cmd;
     }},
    {std::regex(R"(del\s*(\d+)?)"), "del <num>", "Delete breakpoint or watchpoint <num>",
     [](const std::vector<std::string>& groups) -> Command {
         CommandDelete cmd {};
         const std::string& num = groups[0];
         if (!num.empty())
             cmd.num = std::stoi(num);
         return cmd;
     }},
    {std::regex(R"(d\s*(\d+)?)"), "d [<n>]", "Disassemble next <n> instructions (default = 10)",
     [](const std::vector<std::string>& groups) -> Command {
         CommandDisassemble cmd {};
         const std::string& n = groups[0];
         cmd.n = !n.empty() ? std::stoi(n) : 10;
         return cmd;
     }},
    {std::regex(R"(d\s+(\[0-9a-fA-F]{1,4}),([0-9a-fA-F]{1,4}))"), "d <start>,<end>",
     "Disassemble instructions from address <start> to <end>",
     [](const std::vector<std::string>& groups) -> Command {
         CommandDisassembleRange cmd {};
         cmd.from = std::stoi(groups[0], nullptr, 16);
         cmd.to = std::stoi(groups[1], nullptr, 16);
         return cmd;
     }},
    {std::regex(R"(x(?:/(\d+)?(?:([xhbdi])(\d+)?)?)?\s+([0-9a-fA-F]{1,4}))"), "x[/<length><format>] <addr>",
     "Display memory content at <addr> (<format>: x, h[<cols>], b, d, i)",
     [](const std::vector<std::string>& groups) -> Command {
         CommandExamine cmd {};
         const std::string& length = groups[0];
         const std::string& format = groups[1];
         const std::string& formatArg = groups[2];
         const std::string& address = groups[3];
         cmd.length = length.empty() ? 1 : std::stoi(length);
         cmd.format = format.empty() ? 'x' : format[0];
         if (!formatArg.empty())
             cmd.formatArg = stoi(formatArg);
         cmd.address = std::stoi(address, nullptr, 16);
         return cmd;
     }},
    {std::regex(R"(display(?:/(\d+)?(?:([xhbdi])(\d+)?)?)?\s+([0-9a-fA-F]{1,4}))"), "display[/<length><format>] <addr>",
     "Automatically display memory content content at <addr> (<format>: x, h[<cols>], b, d, i)",
     [](const std::vector<std::string>& groups) -> Command {
         CommandDisplay cmd {};
         const std::string& length = groups[0];
         const std::string& format = groups[1];
         const std::string& formatArg = groups[2];
         const std::string& address = groups[3];
         cmd.length = length.empty() ? 1 : std::stoi(length);
         cmd.format = format.empty() ? 'x' : format[0];
         if (!formatArg.empty())
             cmd.formatArg = stoi(formatArg);
         cmd.address = std::stoi(address, nullptr, 16);
         return cmd;
     }},
    {std::regex(R"(undisplay)"), "undisplay", "Undisplay expressions set with display",
     [](const std::vector<std::string>& groups) -> Command {
         CommandUndisplay cmd {};
         return cmd;
     }},
    {std::regex(R"(\.\s*(\d+)?)"), ". [<count>]", "Continue running for <count> PPU dots (default = 1)",
     [](const std::vector<std::string>& groups) -> Command {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return CommandDot {.count = n};
     }},
    {std::regex(R"(s\s*(\d+)?)"), "s [<count>]", "Continue running for <count> instructions (default = 1)",
     [](const std::vector<std::string>& groups) -> Command {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return CommandStep {.count = n};
     }},
    {std::regex(R"(si\s*(\d+)?)"), "si [<count>]", "Continue running for <count> micro-operations (default = 1)",
     [](const std::vector<std::string>& groups) -> Command {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return CommandMicroStep {.count = n};
     }},
    {std::regex(R"(n\s*(\d+)?)"), "n [<count>]",
     "Continue running for <count> instructions at the same stack level (default = 1)",
     [](const std::vector<std::string>& groups) -> Command {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return CommandNext {.count = n};
     }},
    {std::regex(R"(ni\s*(\d+)?)"), "ni [<count>]",
     "Continue running for <count> micro-operations at the same stack level (default = 1)",
     [](const std::vector<std::string>& groups) -> Command {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return CommandMicroNext {.count = n};
     }},
    {std::regex(R"(f\s*(\d+)?)"), "f [<count>]", "Step by <count> frames (default = 1)",
     [](const std::vector<std::string>& groups) -> Command {
         const std::string& count = groups[0];
         uint64_t n = count.empty() ? 1 : std::stoi(count);
         return CommandFrame {.count = n};
     }},
    {std::regex(R"(c)"), "c", "Continue",
     [](const std::vector<std::string>& groups) -> Command {
         return CommandContinue {};
     }},
    {std::regex(R"(trace\s*(on|off)?)"), "trace [on|off]", "Enable/disable state trace (output on standard error)",
     [](const std::vector<std::string>& groups) -> Command {
         const std::string& onoff = groups[0];
         CommandTrace cmd;
         if (!onoff.empty())
             cmd.enabled = onoff == "on";
         return cmd;
     }},
    {std::regex(R"(h(?:elp)?)"), "h", "Display help",
     [](const std::vector<std::string>& groups) -> Command {
         return CommandHelp {};
     }},
    {std::regex(R"(q)"), "q", "Quit",
     [](const std::vector<std::string>& groups) -> Command {
         return CommandQuit {};
     }},
};

static std::optional<Command> parse_command(const std::string& s) {
    for (const auto& command : COMMANDS) {
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
    //    signal(SIGINT, sigint_handler);
    struct sigaction sa {};
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
}

static void detach_sigint_handler() {
    //    signal(SIGINT, SIG_DFL);
    sigaction(SIGINT, nullptr, nullptr);
}

DebuggerFrontendCli::Config DebuggerFrontendCli::Config::makeDefault() {
    return {.sections = {.breakpoints = true,
                         .watchpoints = true,
                         .cpu = true,
                         .ppu = true,
                         .flags = true,
                         .registers = true,
                         .interrupts = true,
                         .io {
                             .joypad = true,
                             .serial = true,
                             .timers = true,
                             .sound = true,
                             .lcd = true,
                         },
                         .code = true}};
}

DebuggerFrontendCli::DebuggerFrontendCli(IDebuggerBackend& backend) :
    backend(backend),
    config(Config::makeDefault()),
    lastCommand("n"),
    trace(),
    observer() {
    backend.attachFrontend(this);
    attach_sigint_handler();
}

DebuggerFrontendCli::~DebuggerFrontendCli() {
    detach_sigint_handler();
}

Debugger::Command DebuggerFrontendCli::pullCommand(Debugger::ExecutionState outcome) {
    struct DisassembleEntry {
        uint16_t address;
        Debugger::Disassemble disassemble;
    };

    auto headerString = [](const std::string& label = "", const std::string& sep = "—") {
        static const size_t WIDTH = 80;
        std::string paddedLabel = label.empty() ? "" : (" " + label + " ");
        size_t w2 = (WIDTH - paddedLabel.size()) / 2;
        size_t w1 = WIDTH - w2 - paddedLabel.size();
        std::string hr1;
        std::string hr2;
        for (size_t i = 0; i < w1; i++)
            hr1 += sep;
        for (size_t i = 0; i < w2; i++)
            hr2 += sep;
        std::stringstream ss;
        ss << termcolor::color<240> << hr1 << termcolor::cyan << paddedLabel << termcolor::color<240> << hr2
           << termcolor::reset;
        return ss.str();
    };

    auto subheaderString = [](const std::string& label = "", const std::string& sep = " ") {
        static const size_t WIDTH = 80;
        std::string paddedLabel = label.empty() ? "" : (" " + label + " ");
        size_t w2 = (WIDTH - paddedLabel.size()) / 2;
        size_t w1 = WIDTH - w2 - paddedLabel.size();
        std::string hr1;
        std::string hr2;
        for (size_t i = 0; i < w1; i++)
            hr1 += sep;
        for (size_t i = 0; i < w2; i++)
            hr2 += sep;
        std::stringstream ss;
        ss << termcolor::color<240> << hr1 << termcolor::green << paddedLabel << termcolor::color<240> << hr2
           << termcolor::reset;
        return ss.str();
    };
    auto coloredBool = [](bool b) {
        std::stringstream ss;
        if (b)
            ss << "1";
        else
            ss << termcolor::color<240> << "0" << termcolor::reset;
        return ss.str();
    };

    auto registerString = [](const std::string& name, uint16_t value) {
        std::stringstream ss;
        ss << termcolor::bold << termcolor::red << name << termcolor::reset << " : " << bin((uint8_t)(value >> 8))
           << " " << bin((uint8_t)(value & 0xFF)) << "  "
           << "(" << hex((uint8_t)(value >> 8)) << " " << hex((uint8_t)(value & 0xFF)) << ")";
        return ss.str();
    };

    auto interruptString = [&coloredBool](const std::string& name, bool IME, bool IE, bool IF) {
        std::stringstream ss;
        ss << termcolor::bold << termcolor::red << name << "  ";
        if (IME && IE && IF)
            ss << termcolor::cyan << "ON " << termcolor::reset << "  ";
        else
            ss << termcolor::color<240> << "OFF" << termcolor::reset << "  ";

        ss << "IE : " << coloredBool(IE) << " | "
           << "IF : " << coloredBool(IF) << termcolor::reset;
        return ss.str();
    };

    auto disassembleEntryCodeString = [&](const DisassembleEntry& entry,
                                          const ICPUDebug::State::Instruction& currentInstruction) {
        std::stringstream ss;
        uint16_t instrStart = entry.address;
        uint16_t instrEnd = instrStart + entry.disassemble.size();
        bool isCurrentInstruction = instrStart <= currentInstruction.address && currentInstruction.address < instrEnd;
        bool isPastInstruction = currentInstruction.address >= instrEnd;

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
                ss << termcolor::reset << termcolor::color<245> << "   M" << (currentInstruction.microop + 1) << "/"
                   << +min << (max != min ? (std::string(":") + std::to_string(+max)) : "") << termcolor::reset;
            }
        }
        return ss.str();
    };

    auto disassembleEntryString = [&](const DisassembleEntry& entry) {
        std::stringstream ss;
        ss << hex(entry.address) << " : " << std::left << std::setw(9) << hex(entry.disassemble) << "   " << std::left
           << std::setw(16) << instruction_mnemonic(entry.disassemble, entry.address);
        return ss.str();
    };

    auto flagString = [&coloredBool](const std::string& name, bool value) {
        std::stringstream ss;
        ss << termcolor::bold << termcolor::red << name << termcolor::reset << " : " << coloredBool(value);
        return ss.str();
    };

    auto breakpointString = [&](const Debugger::Breakpoint& b) {
        std::stringstream ss;
        ss << termcolor::color<13> << "(" << b.id << ") " << hex(b.address);
        auto instruction = backend.getDisassembled(b.address);
        if (instruction) {
            ss << " : " << std::setw(9) << hex(*instruction) << "   " << std::left << std::setw(16)
               << instruction_mnemonic(*instruction, b.address);
        }
        ss << termcolor::reset;
        return ss.str();
    };

    auto watchpointString = [](const Debugger::Watchpoint& w) {
        std::stringstream ss;
        ss << termcolor::yellow << "(" << w.id << ") ";
        if (w.address.from != w.address.to)
            ss << hex(w.address.from) << " - " << hex(w.address.to);
        else
            ss << hex(w.address.from);
        if (w.condition.enabled) {
            if (w.condition.condition.operation == Debugger::Watchpoint::Condition::Operator::Equal) {
                ss << " == " << hex(w.condition.condition.operand);
            }
        }
        if (w.type == Debugger::Watchpoint::Type::Read)
            ss << " (read)";
        if (w.type == Debugger::Watchpoint::Type::Write)
            ss << " (write)";
        if (w.type == Debugger::Watchpoint::Type::ReadWrite)
            ss << " (read/write)";
        if (w.type == Debugger::Watchpoint::Type::Change)
            ss << " (change)";
        ss << termcolor::reset;
        return ss.str();
    };

    auto ioString = [&](uint16_t addr, uint8_t value, int width = 8) {
        std::stringstream ss;
        ss << termcolor::color<244> << hex(addr) << "  " << std::right << termcolor::bold << termcolor::red << std::left
           << std::setw(width) << address_mnemonic(addr) << termcolor::reset << " : " << bin(value);
        return ss.str();
    };

    auto printIo = [&](const uint16_t* addresses, size_t length, size_t columns = 4) {
        size_t i;
        for (i = 0; i < length;) {
            uint16_t addr = addresses[i];
            uint8_t value = backend.readMemory(addr);
            std::cout << ioString(addr, value);
            i++;
            if (i % columns == 0)
                std::cout << std::endl;
            else
                std::cout << "    ";
        }
        if (i % columns != 0)
            std::cout << std::endl;
    };

    auto printHelp = []() {
        auto it = std::max_element(std::begin(COMMANDS), std::end(COMMANDS),
                                   [](const CommandInfo& i1, const CommandInfo& i2) {
                                       return i1.format.length() < i2.format.length();
                                   });
        for (const auto& info : COMMANDS) {
            std::cout << std::left << std::setw(static_cast<int>(it->format.length())) << info.format << " : "
                      << info.help << std::endl;
        }
    };

    auto printInstructions = [&](uint16_t from, size_t n) {
        backend.disassemble(from, n);
        size_t i = 0;
        for (uint32_t address = from; address <= 0xFFFF && i < n;) {
            std::optional<Debugger::Disassemble> disas = backend.getDisassembled(address);
            if (!disas)
                throw std::runtime_error("unexpected");

            DisassembleEntry d = {.address = static_cast<uint16_t>(address), .disassemble = *disas};
            std::cout << disassembleEntryString(d) << std::endl;
            address += disas->size();
            i++;
        }
    };

    auto printMemory = [&](uint16_t from, size_t n, char format, std::optional<uint8_t> formatArg) {
        if (format == 'x') {
            for (size_t i = 0; i < n; i++)
                std::cout << hex(backend.readMemory(from + i)) << " ";
            std::cout << std::endl;
        } else if (format == 'b') {
            for (size_t i = 0; i < n; i++)
                std::cout << bin(backend.readMemory(from + i)) << " ";
            std::cout << std::endl;
        } else if (format == 'd') {
            for (size_t i = 0; i < n; i++)
                std::cout << +backend.readMemory(from + i) << " ";
            std::cout << std::endl;
        } else if (format == 'i') {
            printInstructions(from, n);
        } else if (format == 'h') {
            std::vector<uint8_t> data;
            for (size_t i = 0; i < n; i++)
                data.push_back(backend.readMemory(from + i));
            uint8_t cols = formatArg ? *formatArg : 16;
            std::string hexdump =
                Hexdump().setBaseAddress(from).showAddresses(true).showAscii(false).setNumColumns(cols).hexdump(data);
            std::cout << hexdump << std::endl;
        }
    };

    auto printDisplayEntry = [&printMemory](const DisplayEntry& d) {
        if (std::holds_alternative<DisplayEntry::Examine>(d.expression)) {
            DisplayEntry::Examine dx = std::get<DisplayEntry::Examine>(d.expression);
            std::cout << d.id << ": "
                      << "x/" << d.length << d.format << (d.formatArg ? std::to_string(*d.formatArg) : "") << " "
                      << hex(dx.address) << std::endl;
            printMemory(dx.address, d.length, d.format, d.formatArg);
        }
    };

    auto ppuStateToString = [](IPPUDebug::PPUState state) {
        switch (state) {
        case IPPUDebug::PPUState::HBlank:
            return "HBlank";
        case IPPUDebug::PPUState::VBlank:
            return "VBlank";
        case IPPUDebug::PPUState::OAMScan:
            return "OAMScan";
        case IPPUDebug::PPUState::PixelTransfer:
            return "PixelTransfer";
        }
        return "Unknown";
    };

    auto ppuFetcherStateToString = [](IPPUDebug::FetcherState state) {
        switch (state) {
        case IPPUDebug::FetcherState::Prefetcher:
            return "Prefetcher";
        case IPPUDebug::FetcherState::PixelSliceFetcher:
            return "PixelSliceFetcher";
        case IPPUDebug::FetcherState::Pushing:
            return "Pushing";
        }
        return "Unknown";
    };

    auto printUI = [&]() {
        if (config.sections.breakpoints) {
            const auto& breakpoints = backend.getBreakpoints();
            if (!breakpoints.empty()) {
                std::cout << headerString("breakpoints") << std::endl;
                for (const auto& b : breakpoints)
                    std::cout << breakpointString(b) << std::endl;
            }
        }

        if (config.sections.watchpoints) {
            const auto& watchpoints = backend.getWatchpoints();
            if (!watchpoints.empty()) {
                std::cout << headerString("watchpoints") << std::endl;
                for (const auto& w : watchpoints)
                    std::cout << watchpointString(w) << std::endl;
            }
        }

        auto cpu = backend.getCPUState();
        auto ppu = backend.getPPUState();
        auto lcd = backend.getLCDState();

        uint8_t IE = backend.readMemory(Registers::Interrupts::IE);
        uint8_t IF = backend.readMemory(Registers::Interrupts::IF);

        if (config.sections.cpu) {
            std::cout << headerString("CPU") << std::endl;
            std::cout << termcolor::yellow << "Cycle           :  " << termcolor::reset << cpu.cycles << std::endl;
        }

        if (config.sections.ppu) {

            auto oamEntryToString = [](const PPU::OAMEntryFetchInfo& entry) {
                std::stringstream ss;
                ss << "[" << std::setw(2) << +entry.number << ", (" << std::setw(3) << std::right << +(entry.x - 8)
                   << "," << std::setw(3) << std::right << +(entry.y - 16) << ")]";
                return ss.str();
            };
            std::cout << headerString("PPU") << std::endl;

            std::cout << subheaderString("lcd") << std::endl;
            std::cout << termcolor::yellow << "Position        :  " << termcolor::reset << "(x=" << +lcd.x
                      << ", y=" << +lcd.y << ")" << std::endl;

            std::cout << subheaderString("ppu") << std::endl;
            std::cout << termcolor::yellow << "Cycle           :  " << termcolor::reset << ppu.ppu.cycles << std::endl;

            std::vector<uint8_t> bgFifo;
            for (const auto& p : ppu.ppu.bgFifo)
                bgFifo.push_back(p.colorIndex);

            std::vector<uint8_t> objFifo;
            for (const auto& p : ppu.ppu.objFifo)
                objFifo.push_back(p.colorIndex);

            std::cout << termcolor::yellow << "State           :  " << termcolor::reset
                      << ppuStateToString(ppu.ppu.state) << " (" << ppu.ppu.dots << " dots)" << std::endl;

            std::cout << termcolor::yellow << "Bg Fifo         :  " << termcolor::reset << hex(bgFifo) << std::endl;
            std::cout << termcolor::yellow << "Obj Fifo        :  " << termcolor::reset << hex(objFifo)
                      << termcolor::reset << std::endl;

            std::cout << termcolor::yellow << "LY OAM entries  :" << termcolor::reset << std::endl;
            if (!ppu.ppu.scanlineOamEntries.empty()) {
                int i = 1;
                for (const auto& entry : ppu.ppu.scanlineOamEntries) {
                    std::cout << std::setw(2) << i << ". " << oamEntryToString(entry) << std::endl;
                    i++;
                }
                std::cout << std::endl;
            }

            std::cout << subheaderString("fetcher") << std::endl;
            std::cout << termcolor::yellow << "State           :  " << termcolor::reset
                      << ppuFetcherStateToString(ppu.fetcher.state) << " (" << ppu.fetcher.dots << " dots)"
                      << std::endl;

            std::cout << termcolor::yellow << "Target Fifo     :  " << termcolor::reset
                      << (ppu.fetcher.targetFifo == IPPUDebug::FIFOType::Bg ? "Bg" : "Obj") << std::endl;
            std::vector<uint8_t> fetchedTile;

            for (const auto& p : ppu.fetcher.pixelSliceFetcherTile.data)
                fetchedTile.push_back(p.colorIndex);

            std::cout << termcolor::yellow << "Tile Fetched    :  " << termcolor::reset << hex(fetchedTile);
            if (!fetchedTile.empty())
                std::cout << " (" << hex(ppu.fetcher.pixelSliceFetcherTile.address) << ")";
            std::cout << std::endl;

            std::cout << termcolor::yellow << "Hit OAM entries :" << termcolor::reset << std::endl;

            if (!ppu.fetcher.oamEntriesHit.empty()) {
                int i = 1;
                for (const auto& entry : ppu.fetcher.oamEntriesHit) {
                    std::cout << std::setw(2) << i << ". " << oamEntryToString(entry) << std::endl;
                    i++;
                }
                std::cout << std::endl;
            }
        }

        if (config.sections.flags) {
            std::cout << headerString("flags") << std::endl;
            std::cout << flagString("Z", get_bit<Bits::Flags::Z>(cpu.registers.AF)) << "  |  "
                      << flagString("N", get_bit<Bits::Flags::N>(cpu.registers.AF)) << "  |  "
                      << flagString("H", get_bit<Bits::Flags::H>(cpu.registers.AF)) << "  |  "
                      << flagString("C", get_bit<Bits::Flags::C>(cpu.registers.AF)) << std::endl;
        }

        if (config.sections.registers) {
            std::cout << headerString("registers") << std::endl;
            std::cout << registerString("AF", cpu.registers.AF) << std::endl;
            std::cout << registerString("BC", cpu.registers.BC) << std::endl;
            std::cout << registerString("DE", cpu.registers.DE) << std::endl;
            std::cout << registerString("HL", cpu.registers.HL) << std::endl;
            std::cout << registerString("PC", cpu.registers.PC) << std::endl;
            std::cout << registerString("SP", cpu.registers.SP) << std::endl;
        }

        if (config.sections.interrupts) {
            std::cout << headerString("interrupts") << std::endl;
            std::cout << termcolor::bold << termcolor::red << "IME" << termcolor::reset << " : " << coloredBool(cpu.IME)
                      << std::endl;
            std::cout << termcolor::bold << termcolor::red << "IE" << termcolor::reset << "  : " << bin(IE)
                      << std::endl;
            std::cout << termcolor::bold << termcolor::red << "IF" << termcolor::reset << "  : " << bin(IF)
                      << std::endl;
            std::cout << interruptString("VBLANK", cpu.IME, get_bit<Bits::Interrupts::VBLANK>(IE),
                                         get_bit<Bits::Interrupts::VBLANK>(IF))
                      << std::endl;
            std::cout << interruptString("STAT  ", cpu.IME, get_bit<Bits::Interrupts::STAT>(IE),
                                         get_bit<Bits::Interrupts::STAT>(IF))
                      << std::endl;
            std::cout << interruptString("TIMER ", cpu.IME, get_bit<Bits::Interrupts::TIMER>(IE),
                                         get_bit<Bits::Interrupts::TIMER>(IF))
                      << std::endl;
            std::cout << interruptString("JOYPAD", cpu.IME, get_bit<Bits::Interrupts::JOYPAD>(IE),
                                         get_bit<Bits::Interrupts::JOYPAD>(IF))
                      << std::endl;
            std::cout << interruptString("SERIAL", cpu.IME, get_bit<Bits::Interrupts::SERIAL>(IE),
                                         get_bit<Bits::Interrupts::SERIAL>(IF))
                      << std::endl;
        }

        if (config.sections.io.joypad || config.sections.io.serial || config.sections.io.timers ||
            config.sections.io.sound || config.sections.io.lcd) {
            std::cout << headerString("IO") << std::endl;
            if (config.sections.io.joypad) {
                std::cout << subheaderString("joypad") << std::endl;
                printIo(Registers::Joypad::REGISTERS, sizeof(Registers::Joypad::REGISTERS) / sizeof(uint16_t));
            }
            if (config.sections.io.serial) {
                std::cout << subheaderString("serial") << std::endl;
                printIo(Registers::Serial::REGISTERS, sizeof(Registers::Serial::REGISTERS) / sizeof(uint16_t));
            }
            if (config.sections.io.timers) {
                std::cout << subheaderString("timers") << std::endl;
                printIo(Registers::Timers::REGISTERS, sizeof(Registers::Timers::REGISTERS) / sizeof(uint16_t));
            }
            if (config.sections.io.sound) {
                std::cout << subheaderString("sound") << std::endl;
                printIo(Registers::Sound::REGISTERS, sizeof(Registers::Sound::REGISTERS) / sizeof(uint16_t));
            }
            if (config.sections.io.lcd) {
                std::cout << subheaderString("lcd") << std::endl;
                printIo(Registers::LCD::REGISTERS, sizeof(Registers::LCD::REGISTERS) / sizeof(uint16_t));
            }
        }

        if (config.sections.code) {
            if (cpu.instruction.ISR) {
                std::cout << headerString("code") << std::endl;
                std::cout << termcolor::yellow << "ISR " << interrupt_service_routine_mnemonic(cpu.instruction.address)
                          << "   ";
                std::cout << termcolor::reset << termcolor::color<245> << "   M" << (cpu.instruction.microop + 1) << "/"
                          << 5 << termcolor::reset << std::endl;
            } else {
                std::list<DisassembleEntry> codeView;
                uint8_t n = 0;
                for (int32_t addr = cpu.instruction.address; addr >= 0 && n < 6; addr--) {
                    auto entry = backend.getDisassembled(addr);
                    if (entry) {
                        DisassembleEntry d = {.address = static_cast<uint16_t>(addr), .disassemble = *entry};
                        codeView.push_front(d);
                        n++;
                    }
                }
                n = 0;
                for (int32_t addr = cpu.instruction.address + 1; addr <= 0xFFFF && n < 8; addr++) {
                    auto entry = backend.getDisassembled(addr);
                    if (entry) {
                        DisassembleEntry d = {.address = static_cast<uint16_t>(addr), .disassemble = *entry};
                        codeView.push_back(d);
                        n++;
                    }
                }

                if (!codeView.empty()) {
                    std::cout << headerString("code") << std::endl;
                    std::list<DisassembleEntry>::const_iterator lastEntry;
                    for (auto entry = codeView.begin(); entry != codeView.end(); entry++) {
                        if (entry != codeView.begin()) {
                            if (lastEntry->address + lastEntry->disassemble.size() < entry->address)
                                std::cout << termcolor::color<240> << "  ...." << termcolor::reset << std::endl;
                        }
                        std::cout << disassembleEntryCodeString(*entry, cpu.instruction) << std::endl;
                        lastEntry = entry;
                    }
                }
            }
        }

        if (!displayEntries.empty()) {
            std::cout << headerString("display") << std::endl;
            for (const auto& d : displayEntries) {
                printDisplayEntry(d);
            }
        }

        if (outcome.state == Debugger::ExecutionState::State::BreakpointHit) {
            std::cout << headerString("interruption") << std::endl;
            auto hit = outcome.breakpointHit;
            std::cout << "Triggered breakpoint [" << hit.breakpoint.id << "] at address " << hex(hit.breakpoint.address)
                      << std::endl;
        } else if (outcome.state == Debugger::ExecutionState::State::WatchpointHit) {
            std::cout << headerString("interruption") << std::endl;
            auto hit = outcome.watchpointHit;
            std::cout << "Triggered watchpoint [" << hit.watchpoint.id << "] at address " << hex(hit.address)
                      << std::endl;
            if (hit.accessType == Debugger::WatchpointHit::AccessType::Read) {
                std::cout << "Read at address " << hex(hit.address) << ": " << hex(hit.newValue) << std::endl;
            } else {
                std::cout << "Write at address " << hex(hit.address) << std::endl;
                std::cout << "Old: " << hex(hit.oldValue) << std::endl;
                std::cout << "New: " << hex(hit.newValue) << std::endl;
            }
        } else if (outcome.state == Debugger::ExecutionState::State::Interrupted) {
            std::cout << headerString("interruption") << std::endl;
            std::cout << "User interruption requested" << std::endl;
        }
    };

    std::string command;

    bool invalidateUI = true;

    do {
        if (invalidateUI) {
            invalidateUI = false;
            printUI();
        }

        std::cout << termcolor::bold << termcolor::green << ">> " << termcolor::reset;

        if (observer)
            observer->onReadCommand();

        sigint_trigger = false;
        getline(std::cin, command);

        if (sigint_trigger) { // CTRL+C
            std::cin.clear();
            std::cout << std::endl;
            continue;
        }

        if (std::cin.fail() || std::cin.eof()) // CTRL+D
            return Debugger::CommandAbort();

        if (command.empty())
            command = lastCommand;
        else
            lastCommand = command;

        std::optional<Command> parseResult = parse_command(command);
        if (!parseResult) {
            std::cerr << "Invalid command" << std::endl;
            continue;
        }

        const auto& cmd = *parseResult;

        if (std::holds_alternative<CommandBreakpoint>(cmd)) {
            CommandBreakpoint b = std::get<CommandBreakpoint>(cmd);
            uint32_t id = backend.addBreakpoint(b.address);
            std::cout << "Breakpoint [" << id << "] at " << hex(b.address) << std::endl;
        } else if (std::holds_alternative<CommandWatchpoint>(cmd)) {
            CommandWatchpoint w = std::get<CommandWatchpoint>(cmd);
            std::optional<Debugger::Watchpoint::Condition> cond;
            if (w.condition.enabled)
                cond = w.condition.condition;
            uint32_t id = backend.addWatchpoint(w.type, w.address.from, w.address.to, cond);
            if (w.address.from == w.address.to)
                std::cout << "Watchpoint [" << id << "] at " << hex(w.address.from) << std::endl;
            else
                std::cout << "Watchpoint [" << id << "] at " << hex(w.address.from) << " - " << hex(w.address.to)
                          << std::endl;
        } else if (std::holds_alternative<CommandDelete>(cmd)) {
            CommandDelete del = std::get<CommandDelete>(cmd);
            if (del.num)
                backend.removePoint(*del.num);
            else
                backend.clearPoints();
            invalidateUI = true;
        } else if (std::holds_alternative<CommandDisassemble>(cmd)) {
            CommandDisassemble disas = std::get<CommandDisassemble>(cmd);
            backend.disassemble(backend.getCPUState().instruction.address, disas.n);
            invalidateUI = true;
        } else if (std::holds_alternative<CommandDisassembleRange>(cmd)) {
            CommandDisassembleRange disasRange = std::get<CommandDisassembleRange>(cmd);
            backend.disassembleRange(disasRange.from, disasRange.to);
            invalidateUI = true;
        } else if (std::holds_alternative<CommandExamine>(cmd)) {
            CommandExamine examine = std::get<CommandExamine>(cmd);
            printMemory(examine.address, examine.length, examine.format, examine.formatArg);
        } else if (std::holds_alternative<CommandDisplay>(cmd)) {
            CommandDisplay display = std::get<CommandDisplay>(cmd);
            DisplayEntry d = {.id = static_cast<uint32_t>(displayEntries.size()),
                              .format = display.format,
                              .formatArg = display.formatArg,
                              .length = display.length,
                              .expression = DisplayEntry::Examine {.address = display.address}};
            displayEntries.push_back(d);
            printDisplayEntry(d);
        } else if (std::holds_alternative<CommandUndisplay>(cmd)) {
            displayEntries.clear();
        } else if (std::holds_alternative<CommandDot>(cmd)) {
            CommandDot dot = std::get<CommandDot>(cmd);
            return Debugger::CommandDot {.count = dot.count};
        } else if (std::holds_alternative<CommandStep>(cmd)) {
            CommandStep step = std::get<CommandStep>(cmd);
            return Debugger::CommandStep {.count = step.count};
        } else if (std::holds_alternative<CommandMicroStep>(cmd)) {
            CommandMicroStep step = std::get<CommandMicroStep>(cmd);
            return Debugger::CommandMicroStep {.count = step.count};
        } else if (std::holds_alternative<CommandNext>(cmd)) {
            CommandNext next = std::get<CommandNext>(cmd);
            return Debugger::CommandNext {.count = next.count};
        } else if (std::holds_alternative<CommandMicroNext>(cmd)) {
            CommandMicroNext next = std::get<CommandMicroNext>(cmd);
            return Debugger::CommandMicroNext {.count = next.count};
        } else if (std::holds_alternative<CommandFrame>(cmd)) {
            CommandFrame frame = std::get<CommandFrame>(cmd);
            return Debugger::CommandFrame {.count = frame.count};
        } else if (std::holds_alternative<CommandContinue>(cmd)) {
            return Debugger::CommandContinue();
        } else if (std::holds_alternative<CommandTrace>(cmd)) {
            CommandTrace cmdTrace = std::get<CommandTrace>(cmd);
            if (cmdTrace.enabled)
                trace = *cmdTrace.enabled;
            else
                trace = !trace;
            std::cout << "Trace: " << (trace ? "on" : "off") << std::endl;
        } else if (std::holds_alternative<CommandHelp>(cmd)) {
            printHelp();
        } else if (std::holds_alternative<CommandQuit>(cmd)) {
            return Debugger::CommandAbort();
        }
    } while (true);
}

void DebuggerFrontendCli::onTick(uint64_t tick) {
    if (trace) {
        bool isMcycle = tick % 4 == 0;
        if (isMcycle) {
            Debugger::CPUState cpu = backend.getCPUState();
            if (cpu.instruction.microop == 0) {
                std::string instr;
                if (!cpu.instruction.ISR) {
                    backend.disassemble(cpu.instruction.address, 1);
                    auto disas = backend.getDisassembled(cpu.instruction.address);
                    if (disas)
                        instr = instruction_mnemonic(*disas, cpu.instruction.address);
                    else
                        instr = "unknown";
                } else {
                    instr = "ISR " + hex(cpu.instruction.address);
                }
                std::cerr << "AF:" << hex(cpu.registers.AF) << " BC:" << hex(cpu.registers.BC)
                          << " DE:" << hex(cpu.registers.DE) << " HL:" << hex(cpu.registers.HL)
                          << " SP:" << hex(cpu.registers.SP) << " PC:" << hex((uint16_t)(cpu.registers.PC - 1))
                          << " INSTR: " << instr << std::endl;
            }
        }
    }
    if (sigint_trigger) {
        sigint_trigger = 0;
        backend.interrupt();
    }
}

void DebuggerFrontendCli::setConfig(const DebuggerFrontendCli::Config& cfg) {
    config = cfg;
}

void DebuggerFrontendCli::setObserver(DebuggerFrontendCli::Observer* o) {
    observer = o;
}
