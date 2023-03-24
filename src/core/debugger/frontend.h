#ifndef FRONTEND_H
#define FRONTEND_H

#include "shared.h"

class IDebuggerFrontend {
public:
    virtual ~IDebuggerFrontend() = default;
    virtual Debugger::Command pullCommand(Debugger::ExecutionState state) = 0;
    virtual void onTick() = 0;
};

#endif // FRONTEND_H