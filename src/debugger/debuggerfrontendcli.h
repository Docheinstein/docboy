#ifndef DEBUGGERFRONTENDCLI_H
#define DEBUGGERFRONTENDCLI_H

#include "debuggerfrontend.h"
#include "debuggerbackend.h"
#include <string>
#include <map>

class DebuggerFrontendCli : public DebuggerFrontend {
public:
    explicit DebuggerFrontendCli(DebuggerBackend &backend);
    ~DebuggerFrontendCli() override;
    bool callback() override;

private:
    DebuggerBackend &backend;

    std::string lastCommand;

    struct {
        bool done;
        std::vector<DebuggerBackend::DisassembleEntry> entries;
        std::map<uint16_t, size_t> entriesLocation;
    } disassemble;
};

#endif // DEBUGGERFRONTENDCLI_H