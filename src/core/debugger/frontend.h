#ifndef FRONTEND_H
#define FRONTEND_H

#include "shared.h"
#include <cstdint>

class IDebuggerFrontend {
public:
    virtual ~IDebuggerFrontend() = default;
    virtual Debugger::Command pullCommand(const Debugger::ExecutionState& state) = 0;
    virtual void onTick(uint64_t tick) = 0;
};

#endif // FRONTEND_H