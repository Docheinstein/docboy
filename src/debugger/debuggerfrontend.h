#ifndef DEBUGGERFRONTEND_H
#define DEBUGGERFRONTEND_H

class DebuggerFrontend {
public:
    virtual ~DebuggerFrontend();
    virtual void onFrontend() = 0;
    virtual void onTick();
};

#endif // DEBUGGERFRONTEND_H