#ifndef FRONTEND_H
#define FRONTEND_H

#include <cstdint>
#include "shared.h"

class IDebuggerFrontend {
public:
    virtual ~IDebuggerFrontend() = default;
    virtual Debugger::Command pullCommand(Debugger::ExecutionState state) = 0;
    virtual void onTick(uint64_t tick) = 0;
};

#endif // FRONTEND_H