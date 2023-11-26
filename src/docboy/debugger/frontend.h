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
    struct Config {
        struct {
            bool breakpoints {true};
            bool watchpoints {true};
            bool cpu {true};
            bool ppu {true};
            bool mmu {true};
            bool bus {true};
            bool timers {true};
            bool dma {true};
            struct {
                bool joypad {false};
                bool serial {false};
                bool timers {true};
                bool interrupts {true};
                bool sound {false};
                bool video {true};
            } io {};
            bool code {true};
        } sections {};
    };

    explicit DebuggerFrontend(DebuggerBackend& backend);
    ~DebuggerFrontend();

    Command pullCommand(const ExecutionState& state);
    void onTick(uint64_t tick);

    void setOnPullingCommandCallback(std::function<void()> callback);
    void setOnCommandPulledCallback(std::function<bool(const std::string& s)> callback);

private:
    struct DisplayEntry {
        uint32_t id {};
        MemoryOutputFormat format {};
        std::optional<uint8_t> formatArg {};
        size_t length {};

        struct Examine {
            uint16_t address {};
        };
        std::variant<Examine /*, ...*/> expression;
    };

    struct DisassembleEntry {
        uint16_t address {};
        DisassembledInstruction disassemble {};

        [[nodiscard]] std::string toString() const;
    };

    template <typename FrontendCommandType>
    std::optional<Command> handleCommand(const FrontendCommandType& cmd);

    template <uint8_t color>
    static std::string title(const std::string& title = "", const std::string& sep = "—");

    static std::string header(const std::string& title = "", const std::string& sep = "—");
    static std::string subheader(const std::string& title = "", const std::string& sep = "—");

    static std::string hr(const std::string& sep = "—");

    static std::string boolColored(bool value);
    static std::string flag(const std::string& name, bool value);
    static std::string reg(const std::string& name, uint16_t value);
    static std::string div(const std::string& name, uint16_t value, uint8_t highlightBit, int width = 8);
    static std::string timer(const std::string& name, uint8_t value, int width = 8, int valueWidth = 17);
    static std::string interrupt(const std::string& name, bool IME, bool IE, bool IF);
    static std::string io(uint16_t addrress, uint8_t value, int width = 6);
    static std::string watchpoint(const Watchpoint& breakpoint);
    [[nodiscard]] std::string disassembleEntry(const DisassembleEntry& entry) const;
    [[nodiscard]] std::string breakpoint(const Breakpoint& breakpoint) const;

    static void printHelp();
    void printUI(const ExecutionState& executionState) const;
    void printDisplayEntry(const DisplayEntry& entry) const;
    void printInstructions(uint16_t from, uint32_t n) const;
    void printMemory(uint16_t from, uint32_t n, MemoryOutputFormat fmt, std::optional<uint8_t> fmtArg) const;
    void printIO(const uint16_t* addresses, uint32_t length, uint32_t columns = 4) const;

    DebuggerBackend& backend;
    const Core& core;
    const GameBoy& gb;

    Config config {};
    std::string lastCmdline;
    std::vector<DisplayEntry> displayEntries;
    uint32_t trace {};

    uint16_t numAutoDisassemble {};
    bool reprintUI {};

    std::function<void()> onPullingCommandCallback {};
    std::function<bool(const std::string& cmd)> onCommandPulledCallback {};
};

#endif // FRONTEND_H