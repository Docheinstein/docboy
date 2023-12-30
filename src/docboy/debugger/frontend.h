#ifndef FRONTEND_H
#define FRONTEND_H

#include "shared.h"
#include <functional>
#include <optional>
#include <string>

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

    Command pullCommand(const ExecutionState& state);
    void onTick(uint64_t tick);

    void setOnPullingCommandCallback(std::function<void()> callback);
    void setOnCommandPulledCallback(std::function<bool(const std::string& s)> callback);

private:
    struct DisplayEntry {
        uint32_t id {};

        struct Examine {
            MemoryOutputFormat format {};
            std::optional<uint8_t> formatArg {};
            uint16_t address {};
            size_t length {};
            bool raw;
        };

        std::variant<Examine /*, ...*/> expression;
    };

    template <typename FrontendCommandType>
    std::optional<Command> handleCommand(const FrontendCommandType& cmd);

    void printUI(const ExecutionState& executionState) const;

    [[nodiscard]] std::string dumpMemory(uint16_t from, uint32_t n, MemoryOutputFormat fmt,
                                         std::optional<uint8_t> fmtArg, bool raw) const;
    [[nodiscard]] std::string dumpDisplayEntry(const DisplayEntry& d) const;

    DebuggerBackend& backend;
    const Core& core;
    const GameBoy& gb;

    std::string lastCmdline;
    std::vector<DisplayEntry> displayEntries;
    uint32_t trace {};

    uint16_t autoDisassembleNextInstructions {10};
    bool reprintUI {};

    std::function<void()> onPullingCommandCallback {};
    std::function<bool(const std::string& cmd)> onCommandPulledCallback {};
};

#endif // FRONTEND_H