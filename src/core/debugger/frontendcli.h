#ifndef FRONTENDCLI_H
#define FRONTENDCLI_H

#include "frontend.h"
#include <string>
#include <variant>

class IDebuggerBackend;

class DebuggerFrontendCli : public IDebuggerFrontend {
public:
    struct Config {
        struct {
            bool breakpoints;
            bool watchpoints;
            bool cpu;
            bool ppu;
            bool flags;
            bool registers;
            bool interrupts;
            struct {
                bool joypad;
                bool serial;
                bool timers;
                bool sound;
                bool lcd;
            } io;
            bool code;
        } sections;

        static Config makeDefault();
    };

    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onReadCommand() = 0;
    };

    explicit DebuggerFrontendCli(IDebuggerBackend& backend);
    ~DebuggerFrontendCli() override;

    void setConfig(const Config& config);

    void setObserver(Observer* observer);

    Debugger::Command pullCommand(const Debugger::ExecutionState& outcome) override;
    void onTick(uint64_t tick) override;

private:
    IDebuggerBackend& backend;

    Config config;

    std::string lastCommand;

    struct DisplayEntry {
        uint32_t id;
        char format;
        std::optional<uint8_t> formatArg;
        size_t length;

        struct Examine {
            uint16_t address;
        };
        std::variant<Examine /*, ...*/> expression;
    };

    std::vector<DisplayEntry> displayEntries;

    bool trace;

    Observer* observer;
};

#endif // FRONTENDCLI_H