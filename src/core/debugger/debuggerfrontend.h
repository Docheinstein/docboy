#ifndef DEBUGGERFRONTEND_H
#define DEBUGGERFRONTEND_H

#include "debuggerbackend.h"

class IDebuggerFrontend {
public:
    virtual ~IDebuggerFrontend() = default;
    virtual DebuggerBackend::Command pullCommand(DebuggerBackend::ExecutionState state) = 0;
    virtual void onTick() = 0;
};

#endif // DEBUGGERFRONTEND_H