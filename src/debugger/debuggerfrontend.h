#ifndef DEBUGGERFRONTEND_H
#define DEBUGGERFRONTEND_H

class DebuggerFrontend {
public:
    virtual ~DebuggerFrontend();
    virtual bool callback() = 0;
};

#endif // DEBUGGERFRONTEND_H