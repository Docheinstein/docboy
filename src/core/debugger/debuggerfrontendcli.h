#ifndef DEBUGGERFRONTENDCLI_H
#define DEBUGGERFRONTENDCLI_H

#include "debuggerfrontend.h"
#include <string>
#include <map>
#include <variant>

class DebuggerFrontendCli : public IDebuggerFrontend {
public:
    explicit DebuggerFrontendCli(IDebuggerBackend &backend);
    ~DebuggerFrontendCli() override;
    DebuggerBackend::Command pullCommand(DebuggerBackend::ExecutionState outcome) override;
    void onTick() override;

private:
    IDebuggerBackend &backend;

    std::string lastCommand;

    struct DisplayEntry {
        uint32_t id;
        char format;
        size_t length;

        struct Examine {
            uint16_t address;
        };
        std::variant<Examine /*, ...*/> expression;
    };

    std::vector<DisplayEntry> displayEntries;
};

#endif // DEBUGGERFRONTENDCLI_H