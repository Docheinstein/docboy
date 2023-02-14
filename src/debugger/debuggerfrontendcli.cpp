#include "debuggerfrontendcli.h"
#include "debuggerbackend.h"
#include <iostream>
#include "termcolor/termcolor.h"
#include "utils/strutils.h"
#include "utils/binutils.h"
#include "instructions.h"
#include <sstream>
#include <vector>

DebuggerFrontendCli::~DebuggerFrontendCli() {

}

DebuggerFrontendCli::DebuggerFrontendCli(DebuggerBackend &backend)
    : backend(backend), lastCommand("n"), disassemble() {

}

bool DebuggerFrontendCli::callback() {
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

    auto registers = backend.getRegisters();
    DebuggerBackend::Instruction instruction = backend.getCurrentInstruction();

    if (!disassemble.done) {
        disassemble.done = true;
        disassemble.entries = backend.disassemble(0x100, 0x8000);
        size_t i = 0;
        for (const auto& entry : disassemble.entries) {
            for (int j = 0; j < entry.instruction.size(); ++j) {
                disassemble.entriesLocation[entry.address + j] = i;
            }
            ++i;
        }
    }

    auto disassembleEntryString = [&, instruction](const DebuggerBackend::DisassembleEntry &entry) {
        std::stringstream ss;
        uint16_t instrStart = entry.address;
        uint16_t instrEnd = instrStart + entry.instruction.size();
        bool isCurrentInstruction = instrStart <= instruction.address && instruction.address < instrEnd;
        bool isPastInstruction = instruction.address >= instrEnd;

        if (backend.hasBreakPoint(entry.address))
            ss << termcolor::bold << termcolor::red << "• " << termcolor::reset;
        else
            ss << "  ";

        if (isCurrentInstruction)
            ss << termcolor::bold << termcolor::green;
        else if (isPastInstruction)
            ss << termcolor::color<240>;
        ss
            << hex(entry.address) << "  :  " << std::left << std::setw(9) << hex(entry.instruction) << "   "
            << std::left << std::setw(16) << instruction_to_string(entry.instruction) << "  ";

        if (isCurrentInstruction)
            ss
                << termcolor::color<245>
                << "M" << (instruction.microop + 1) << "/" << +entry.info.duration.min
                << (entry.info.duration.max != entry.info.duration.min ? (std::string(":") + std::to_string(+entry.info.duration.max)) : "")
                << termcolor::reset;
        return ss.str();
    };


    std::cout << headerString("registers") << std::endl;
    std::cout << registerString("AF", registers.AF) << std::endl;
    std::cout << registerString("BC", registers.BC) << std::endl;
    std::cout << registerString("DE", registers.DE) << std::endl;
    std::cout << registerString("HL", registers.HL) << std::endl;
    std::cout << registerString("PC", registers.PC) << std::endl;
    std::cout << registerString("SP", registers.SP) << std::endl;

    std::cout << headerString("code") << std::endl;
//    auto currentInstruction = disassembleV.find(PC);
    auto it = disassemble.entriesLocation.find(instruction.address);
    if (it != disassemble.entriesLocation.end()) {
        int loc = static_cast<int>(it->second);
        int from = std::max(0, loc - 5);
        int to = std::min(loc + 5, static_cast<int>(disassemble.entries.size()));
        for (auto i = from; i < to; i++)
            std::cout << disassembleEntryString(disassemble.entries[i]) << std::endl;
    }

    std::cout << headerString() << std::endl;

    std::cout
        << termcolor::magenta
        << "[ b XXXX : Breakpoint | s : Step | n : Next | r : Run ]"
        << termcolor::reset
        << std::endl;

    std::string command;

    getline(std::cin, command);

    if (command.empty())
        command = lastCommand;

    std::vector<std::string> tokens;
    split(command, std::back_inserter(tokens));

    if (tokens.empty())
        return false;

    auto cmd = tokens[0];
    if (cmd.starts_with("b")) {
        if (tokens.size() >= 2) {
            // TODO: check param
            auto addr = std::stoi(tokens[1], nullptr, 16);
            backend.toggleBreakPoint(addr);
        }
    } else if (cmd.starts_with("s")) {
        backend.step();
    } else if (cmd.starts_with("n")) {
        backend.next();
    } else if (cmd.starts_with("r")) {
        backend.run();
    }

    lastCommand = command;
    return false;
}
