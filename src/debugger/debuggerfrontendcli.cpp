#include "debuggerfrontendcli.h"
#include "debuggerbackend.h"
#include <iostream>
#include "termcolor/termcolor.h"
#include "utils/strutils.h"
#include "utils/binutils.h"
#include "instructions.h"
#include <sstream>
#include <vector>
#include <list>
#include <csignal>

static volatile sig_atomic_t sigint_trigger = 0;

static void sigint_handler(int signum) {
    sigint_trigger = 1;
}

static void attach_sigint_handler() {
    signal(SIGINT, sigint_handler);
}

static void detach_sigint_handler() {
    signal(SIGINT, SIG_DFL);
}

DebuggerFrontendCli::~DebuggerFrontendCli() {
}

DebuggerFrontendCli::DebuggerFrontendCli(DebuggerBackend &backend)
    : backend(backend), lastCommand("n") {

}

void DebuggerFrontendCli::onFrontend() {
    auto headerString = [](const std::string &label = "") {
        static const size_t WIDTH = 80;
        std::string paddedLabel = label.empty() ? "" : (" " + label + " ");
        size_t w2 = (WIDTH - paddedLabel.size()) / 2;
        size_t w1 = WIDTH - w2 - paddedLabel.size();
        std::string hr1;
        std::string hr2;
        for (auto i = 0; i < w1; i++)
            hr1 += "—";
        for (auto i = 0; i < w2; i++)
            hr2 += "—";
        std::stringstream ss;
        ss
            << termcolor::color<240> << hr1
            << termcolor::cyan << paddedLabel
            << termcolor::color<240> << hr2
            << termcolor::reset;
        return ss.str();
    };

    auto registerString = [](const std::string &name, uint16_t value) {
        std::stringstream ss;
        ss
            << termcolor::bold << termcolor::red << name << termcolor::reset << "  :  "
            << bin((uint8_t) (value >> 8)) << " " << bin((uint8_t) (value & 0xFF)) << "  "
            << "(" << hex((uint8_t) (value >> 8)) << " " << hex((uint8_t) (value & 0xFF)) << ")";
        return ss.str();
    };

    auto disassembleEntryString = [&](
            const DebuggerBackend::DisassembleEntry &entry,
            DebuggerBackend::Instruction currentInstruction) {
        std::stringstream ss;
        uint16_t instrStart = entry.address;
        uint16_t instrEnd = instrStart + entry.instruction.size();
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
        ss
            << hex(entry.address) << "  :  " << std::left << std::setw(9) << hex(entry.instruction) << "   "
            << std::left << std::setw(16) << instruction_to_string(entry.instruction);

        if (isCurrentInstruction) {
            auto info = instruction_info(entry.instruction);
            if (info) {
                ss
                    << termcolor::reset
                    << termcolor::color<245>
                    << "   M"
                    << (currentInstruction.microop + 1) << "/" << +info->duration.min
                    << (info->duration.max != info->duration.min ?
                            (std::string(":") + std::to_string(+info->duration.max)) : "")
                    << termcolor::reset;
            }
        }
        return ss.str();
    };

    auto flagString = [](const std::string &name, bool value) {
        std::stringstream ss;
        ss
            << termcolor::bold << termcolor::red << name << termcolor::reset
            << " : " << +value;
        return ss.str();
    };

    auto breakpointString = [&](const DebuggerBackend::Breakpoint &b) {
        std::stringstream ss;
        ss
            << termcolor::color<13>
            << "(" << b.id << ") "
            << hex(b.address);
        const auto &entry = backend.getCode(b.address);
        if (entry) {
            ss
                << "  :  "
                << std::setw(9) << hex(entry->instruction) << "   "
                << std::left << std::setw(16) << instruction_to_string(entry->instruction);
        }
//        auto disasLoc = disassemble.entriesLocation.find(b.address);
//        if (disasLoc != disassemble.entriesLocation.end()) {
//            auto entry = disassemble.entries[disasLoc->second];
//            ss
//                << "  :  "
//                << std::setw(9) << hex(entry.instruction) << "   "
//                << std::left << std::setw(16) << instruction_to_string(entry.instruction);
//        }
        ss << termcolor::reset;
        return ss.str();
    };

    auto watchpointString = [](const DebuggerBackend::Watchpoint &w) {
        std::stringstream ss;
        ss
            << termcolor::yellow
            << "(" << w.id << ") ";
        if (w.from != w.to)
            ss << hex(w.from) << " - " << hex(w.to);
        else
            ss << hex(w.from);
        if (w.condition) {
            if (w.condition->operation == DebuggerBackend::Watchpoint::Condition::Operator::Equal) {
                ss << " == " << hex(w.condition->operand);
            }
        }
        if (w.type == DebuggerBackend::Watchpoint::Type::Read)
            ss << " (read)";
        if (w.type == DebuggerBackend::Watchpoint::Type::Write)
            ss << " (write)";
        if (w.type == DebuggerBackend::Watchpoint::Type::ReadWrite)
            ss << " (read/write)";
        if (w.type == DebuggerBackend::Watchpoint::Type::Change)
            ss << " (change)";
        ss << termcolor::reset;
        return ss.str();
    };

    auto cyclepointString = [&](const DebuggerBackend::Cyclepoint &c) {
        std::stringstream ss;
        ss
            << termcolor::color<13>
            << "(" << c.id << ") "
            << c.n;
        ss << termcolor::reset;
        return ss.str();
    };

    std::optional<DebuggerBackend::ExecResult> execResult;
//    std::optional<DebuggerBackend::WatchpointTrigger> triggeredWatchpoint;

    auto printUI = [&](
            DebuggerBackend::RegistersSnapshot registers,
            DebuggerBackend::FlagsSnapshot flags,
            DebuggerBackend::Instruction currentInstruction) {

        const auto &breakpoints = backend.getBreakpoints();
        if (!breakpoints.empty()) {
            std::cout << headerString("breakpoints") << std::endl;
            for (const auto &b : breakpoints)
                std:: cout << breakpointString(b) << std::endl;
        }

        const auto &watchpoints = backend.getWatchpoints();
        if (!watchpoints.empty()) {
            std::cout << headerString("watchpoints") << std::endl;
            for (const auto &w : watchpoints)
                std:: cout << watchpointString(w) << std::endl;
        }

        const auto &cyclepoints = backend.getCyclepoints();
        if (!cyclepoints.empty()) {
            std::cout << headerString("cyclepoints") << std::endl;
            for (const auto &c : cyclepoints)
                std:: cout << cyclepointString(c) << std::endl;
        }

        std::cout << headerString("flags") << std::endl;
        std::cout
            << flagString("Z", flags.Z) << "  |  "
            << flagString("N", flags.N) << "  |  "
            << flagString("H", flags.H) << "  |  "
            << flagString("C", flags.C) << std::endl;
        std::cout << headerString("registers") << std::endl;
        std::cout << registerString("AF", registers.AF) << std::endl;
        std::cout << registerString("BC", registers.BC) << std::endl;
        std::cout << registerString("DE", registers.DE) << std::endl;
        std::cout << registerString("HL", registers.HL) << std::endl;
        std::cout << registerString("PC", registers.PC) << std::endl;
        std::cout << registerString("SP", registers.SP) << std::endl;

        std::cout << headerString("state") << std::endl;
        std::cout << termcolor::yellow << "Cycle    :  " << termcolor::reset << backend.getCurrentCycle() << std::endl;
        std::cout << termcolor::yellow << "M-cycle  :  " << termcolor::reset << backend.getCurrentMcycle() << std::endl;

        std::list<DebuggerBackend::DisassembleEntry> codeView;
        uint8_t n = 0;
        for (int32_t addr = currentInstruction.address; addr >= 0 && n < 10; addr--) {
            auto entry = backend.getCode(addr);
            if (entry) {
                codeView.push_front(*entry);
                n++;
            }
        }
        n = 0;
        for (int32_t addr = currentInstruction.address + 1; addr <= 0xFFFF && n < 10; addr++) {
            auto entry = backend.getCode(addr);
            if (entry) {
                codeView.push_back(*entry);
                n++;
            }
        }

        if (!codeView.empty()) {
            std::cout << headerString("code") << std::endl;
            std::list<DebuggerBackend::DisassembleEntry>::const_iterator lastEntry;
            for (auto entry = codeView.begin(); entry != codeView.end(); entry++) {
                if (entry != codeView.begin()) {
                    if (lastEntry->address + lastEntry->instruction.size() < entry->address)
                        std::cout << termcolor::color<240> << "  ...." << termcolor::reset << std::endl;
                }
                std::cout << disassembleEntryString(*entry, currentInstruction) << std::endl;
                lastEntry = entry;
            }
        }

        if (!display.empty()) {
            std::cout << headerString("display") << std::endl;
            for (const auto &expr : display) {
                if (expr.type == DisplayExpression::Type::X) {
                    std::cout << ">> x/" << expr.n << " " << hex(expr.address) << std::endl;
                    for (int i = 0; i < expr.n; i++)
                        std::cout << hex(backend.readMemory(expr.address + i)) << " ";
                    std::cout << std::endl;
                }
            }
        }

//        const auto &disas = disassemble[currentInstruction.address];
//            int from = std::max(0, currentInstruction.address - 8);
//            int to = std::min(currentInstruction.address + 8, 0xFFFF);
//        std::vector<int> beforeEntries;
//        std::vector<int> afterEntries;
//        for (int32_t addr = currentInstruction.address; addr >= 0 && beforeEntries.size() < 8; addr--) {
//            if (disassemble[addr].valid) {
//                beforeEntries.push_back(addr);
//            }
//        }
//        for (int32_t addr = currentInstruction.address + 1; addr < 0xFFFF && afterEntries.size() < 8; addr++) {
//            if (disassemble[addr].valid) {
//                afterEntries.push_back(addr);
//            }
//        }
//        for (auto it = beforeEntries.rbegin(); it != beforeEntries.rend(); it++) {
//            std::cout << disassembleEntryString(disassemble[*it], currentInstruction) << std::endl;
//        }
//        for (auto it = afterEntries.begin(); it != afterEntries.end(); it++) {
//            std::cout << disassembleEntryString(disassemble[*it], currentInstruction) << std::endl;
//        }
//        auto it = disassemble.entriesLocation.find(currentInstruction.address);
//        if (it != disassemble.entriesLocation.end()) {
//            int loc = static_cast<int>(it->second);
//            int from = std::max(0, loc - 8);
//            int to = std::min(loc + 8, static_cast<int>(disassemble.entries.size()));
//            for (auto i = from; i < to; i++)
//                std::cout << disassembleEntryString(disassemble.entries[i], currentInstruction) << std::endl;
//        }

        if (execResult) {
            std::cout << headerString("interruption") << std::endl;
            if (execResult->reason == DebuggerBackend::ExecResult::EndReason::Breakpoint) {
                auto trigger = execResult->breakpointTrigger;
                std::cout << "Triggered breakpoint [" << trigger.breakpoint.id << "] at address " << hex(trigger.breakpoint.address) << std::endl;
            } else if (execResult->reason == DebuggerBackend::ExecResult::EndReason::Watchpoint) {
                auto trigger = execResult->watchpointTrigger;
                std::cout << "Triggered watchpoint [" << trigger.watchpoint.id << "] at address " << hex(trigger.address) << std::endl;
                if (trigger.type == DebuggerBackend::WatchpointTrigger::Type::Read) {
                    std::cout << "Read at address " << trigger.address << ": " << trigger.newValue << std::endl;
                } else {
                    std::cout << "Write at address " << hex(trigger.address) << std::endl;
                    std::cout << "Old: " << hex(trigger.oldValue) << std::endl;
                    std::cout << "New: " << hex(trigger.newValue) << std::endl;
                }
            } else if (execResult->reason == DebuggerBackend::ExecResult::EndReason::Cyclepoint) {
                auto trigger = execResult->cyclepointTrigger;
                std::cout << "Triggered cyclepoint [" << trigger.cyclepoint.id << "] at cycle " << trigger.cyclepoint.n << std::endl;
            } else if (execResult->reason == DebuggerBackend::ExecResult::EndReason::Interrupted) {
                std::cout << "User interruption requested" << std::endl;
            }
            execResult = std::nullopt;
        }

        std::cout << headerString() << std::endl;

        std::cout
            << termcolor::magenta
            <<  "[ b XXXX : Breakpoint | w[/r|a] XXXX[-XXXX] [== XX] : Watchpoint | x[/<n>] XXXX : eXamine memory ]\n"
            <<  "[ k <n> : Cycle breakpoint ]\n"
                "[ del [<id>]: Delete breakpoint/watchpoint ]\n"
                "[ d [<n>] : Disassemble  | d [<start>,<end>] : Disassemble range ]\n"
                "[ display <expr> : Display | undisplay : Undisplay]\n"
                "[ s : Step | n : Next | c : Continue | r : Run ]"
            << termcolor::reset
            << std::endl;
    };

//    if (!disassemble.done) {
//        disassemble.done = true;
//        disassemble.entries = backend.disassemble(0x100, 0xDFFF);
//        size_t i = 0;
//        for (const auto& entry : disassemble.entries) {
//            for (int j = 0; j < entry.instruction.size(); ++j) {
//                disassemble.entriesLocation[entry.address + j] = i;
//            }
//            ++i;
//        }
//    }

    std::string command;

    bool updateUI = true;


    do {
        DebuggerBackend::Instruction instruction = backend.getCurrentInstruction();

//        auto doDisassemble = [this](uint16_t address) {
//            // TODO: this should be done also on run. Maybe make frontend and backend more monolithic,
//            //  or provide callback in frontend
//            auto &disas = disassemble[address];
//            disas.entry = backend.disassemble(address);
//            if (!disas.entry.instruction.empty()) {
//                disas.valid = true;
//                uint8_t opcode = disas.entry.instruction[0];
//                if (opcode == 0xCB) {
//                    if (disas.entry.instruction.size() >= 2) {
//                        disas.info = INSTRUCTIONS_CB[disas.entry.instruction[1]];
//                    } else {
//                        disas.valid = false;
//                    }
//                } else {
//                    disas.info = INSTRUCTIONS[opcode];
//                }
//            } else {
//                disas.valid = false;
//            }
//            return disas;
//        };
//
//        if (instruction.microop == 0) {
//            doDisassemble(instruction.address);
//        }

        if (updateUI) {
            auto registers = backend.getRegisters();
            auto flags = backend.getFlags();
            printUI(registers, flags, instruction);
            updateUI = false;
        }

        std::cout << termcolor::bold << termcolor::green << ">> " << termcolor::reset;
//        command = readline(">> ");
        getline(std::cin, command);

        if (command.empty())
            command = lastCommand;

        std::vector<std::string> tokens;
        split(command, std::back_inserter(tokens));

        if (tokens.empty())
            return;

        auto cmd = tokens[0];
        if (cmd.starts_with("b")) {
            if (tokens.size() >= 2) {
                // TODO: check param
                uint16_t addr = std::stoi(tokens[1], nullptr, 16);
                uint32_t id = backend.addBreakpoint(addr);
                std::cout << "Breakpoint [" << id << "] at " << hex(addr) << std::endl;
            }
        } else if (cmd.starts_with("w")) {
            if (tokens.size() >= 2) {
                // TODO: check param
                std::vector<std::string> wTokens;
                split(tokens[0], std::back_inserter(wTokens), '/');
                char mode = 0;
                if (wTokens.size() > 1) {
                    mode = wTokens[1][0];
                }

                std::vector<std::string> wParamTokens;
                split(tokens[1], std::back_inserter(wParamTokens), '-');
                uint16_t from = std::stoi(wParamTokens[0], nullptr, 16);
                uint16_t to = wParamTokens.size() > 1 ? std::stoi(wParamTokens[1], nullptr, 16) : from;
                std::optional<DebuggerBackend::Watchpoint::Condition> cond;
                if (tokens.size() >= 3) {
                    auto operation = tokens[2];
                    auto operand = tokens[3];
                    if (operation == "==") {
                        cond = DebuggerBackend::Watchpoint::Condition();
                        cond->operation = DebuggerBackend::Watchpoint::Condition::Operator::Equal;
                        cond->operand = std::stoi(operand, nullptr, 16);
                    }
                }

                DebuggerBackend::Watchpoint::Type type;
                if (mode == 'r') {
                    type = DebuggerBackend::Watchpoint::Type::Read;
                } else if (mode == 'a') {
                    type = DebuggerBackend::Watchpoint::Type::ReadWrite;
                } else /* default */{
                    type = DebuggerBackend::Watchpoint::Type::Change;
                }
                auto id = backend.addWatchpoint(type, from, to, cond);
                if (from == to)
                    std::cout << "Watchpoint [" << id << "] at " << hex(from) << std::endl;
                else
                    std::cout << "Watchpoint [" << id << "] at " << hex(from) << " - " << hex(to) << std::endl;
            }
        } else if (cmd.starts_with("x")) {
            if (tokens.size() >= 2) {
                // TODO: check param
                uint16_t addr = std::stoi(tokens[1], nullptr, 16);
                std::vector<std::string> xTokens;
                split(tokens[0], std::back_inserter(xTokens), '/');
                int n = 1;
                if (xTokens.size() > 1) {
                    n = std::stoi(xTokens[1]);
                }
                for (int i = 0; i < n; i++)
                    std::cout << hex(backend.readMemory(addr + i)) << " ";
                std::cout << std::endl;
            }
        } else if (cmd.starts_with("display")) {
            if (tokens.size() >= 2) {
                if (tokens[1].starts_with("x")) {
                    if (tokens.size() >= 3) {
                        // TODO: check param
                        uint16_t addr = std::stoi(tokens[2], nullptr, 16);
                        std::vector<std::string> xTokens;
                        split(tokens[1], std::back_inserter(xTokens), '/');
                        int n = 1;
                        if (xTokens.size() > 1) {
                            n = std::stoi(xTokens[1]);
                        }
                        DisplayExpression expr {};
                        expr.type = DisplayExpression::Type::X;
                        expr.address = addr;
                        expr.n = n;
                        display.push_back(expr);
                    }
                }
            }
        } else if (cmd.starts_with("del")) {
            if (tokens.size() > 1) {
                uint32_t id = std::stoi(tokens[1]);
                backend.removePoint(id);
            } else {
                backend.clearPoints();
            }
        } else if (cmd.starts_with("d")) {
            if (tokens.size() > 1) {
                std::vector<std::string> dTokens;
                split(tokens[1], std::back_inserter(dTokens), ',');
                if (dTokens.size() == 1) {
                    int n = std::stoi(dTokens[0]);
                    backend.disassemble(instruction.address, n);
                } else if (dTokens.size() == 2) {
                    uint16_t from = std::stoi(dTokens[0], nullptr, 16);
                    uint16_t to = std::stoi(dTokens[1], nullptr, 16);
                    backend.disassembleRange(from, to);
                }
            } else {
                backend.disassemble(instruction.address, 10);
            }
            updateUI = true;
        } else if (cmd.starts_with("s")) {
            execResult = backend.step();
            updateUI = true;
        } else if (cmd.starts_with("n")) {
            execResult = backend.next();
            updateUI = true;
        } else if (cmd.starts_with("c") || cmd.starts_with("r")) {
            if (cmd.starts_with("r"))
                backend.reset();

            attach_sigint_handler();
            execResult = backend.continue_();
            if (execResult->reason == DebuggerBackend::ExecResult::EndReason::Interrupted)
                sigint_trigger = 0;
            detach_sigint_handler();
            updateUI = true;
        } else if (cmd.starts_with("k")) {
            if (tokens.size() >= 2) {
                // TODO: check param
                uint64_t n = std::stoi(tokens[1]);
                uint32_t id = backend.addCyclepoint(n);
                std::cout << "Cyclepoint [" << id << "] at " << n << std::endl;
            }
        } else if (cmd.starts_with("m")) {

        }

        lastCommand = command;
    } while (true);
}

void DebuggerFrontendCli::onTick() {
    if (sigint_trigger) {
        sigint_trigger = 0;
        backend.interrupt();
    }
}
