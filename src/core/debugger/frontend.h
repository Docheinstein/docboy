#ifndef FRONTEND_H
#define FRONTEND_H

#include "backend.h"

class IDebuggerFrontend {
public:
    virtual ~IDebuggerFrontend() = default;
    virtual DebuggerBackend::Command pullCommand(DebuggerBackend::ExecutionState state) = 0;
    virtual void onTick() = 0;
};

#endif // FRONTEND_H