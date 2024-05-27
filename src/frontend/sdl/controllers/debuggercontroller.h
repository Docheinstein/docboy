#ifndef DEBUGGERCONTROLLER_H
#define DEBUGGERCONTROLLER_H

class Window;
class Core;
class CoreController;

class DebuggerController {
public:
    DebuggerController(Window& window, Core& core, CoreController& coreController);

    bool isDebuggerAttached() const;
    bool attachDebugger(bool proceedExecution = false);
    bool detachDebugger();

private:
    Window& window;
    Core& core;
    CoreController& coreController;
};

#endif // DEBUGGERCONTROLLER_H
