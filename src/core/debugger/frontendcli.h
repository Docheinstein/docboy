#ifndef FRONTENDCLI_H
#define FRONTENDCLI_H

#include "frontend.h"
#include <string>
#include <variant>

class IDebuggerBackend;

class DebuggerFrontendCli : public IDebuggerFrontend {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onReadCommand() = 0;
    };

    explicit DebuggerFrontendCli(IDebuggerBackend &backend);
    ~DebuggerFrontendCli() override;

    void setObserver(Observer *observer);

    Debugger::Command pullCommand(Debugger::ExecutionState outcome) override;
    void onTick() override;

private:
    IDebuggerBackend &backend;

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

    Observer *observer;
};

#endif // FRONTENDCLI_H