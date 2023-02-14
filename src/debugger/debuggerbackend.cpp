#include "debuggerbackend.h"

DebuggerBackend::~DebuggerBackend() {

}

void DebuggerBackend::toggleBreakPoint(uint16_t addr) {
    if (hasBreakPoint(addr))
        unsetBreakPoint(addr);
    else
        setBreakPoint(addr);
}
