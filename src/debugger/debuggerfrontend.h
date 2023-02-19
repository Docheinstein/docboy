#ifndef DEBUGGERFRONTEND_H
#define DEBUGGERFRONTEND_H

#include "debuggerbackend.h"

class DebuggerFrontend {
public:
    virtual ~DebuggerFrontend();
    virtual bool callback() = 0;
    virtual bool requiresInterruption() = 0;
};

#endif // DEBUGGERFRONTEND_H