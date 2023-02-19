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
    bool requiresInterruption() override;

private:
    DebuggerBackend &backend;

    std::string lastCommand;
//
//    struct {
//        bool done;
//        std::vector<DebuggerBackend::DisassembleEntry> entries;
//        std::map<uint16_t, size_t> entriesLocation;
//    } disassemble;
//    struct DisassembleEntry {
//        bool valid;
//        DebuggerBackend::DisassembleEntry entry;
//        InstructionInfo info;
//    };

//    DisassembleEntry disassemble[0xFFFF];
//    std::vector<DebuggerBackend::Breakpoint> breakpoints;
//    std::vector<DebuggerBackend::Watchpoint> watchpoints;
//
//    void setBreakpoint(DebuggerBackend::Breakpoint b);
//    void unsetBreakpoint(uint16_t addr);
//    void setWatchpoint(DebuggerBackend::Watchpoint b);
//    void unsetWatchpoint(uint16_t addr);

    struct DisplayExpression {
        enum class Type {
            X
        };
        Type type;
        union {
            struct {
                uint16_t address;
                uint16_t n;
            };
        };

    };
    std::vector<DisplayExpression> display;
};

#endif // DEBUGGERFRONTENDCLI_H