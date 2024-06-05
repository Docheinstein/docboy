#ifndef DEBUGGERCONTROLLER_H
#define DEBUGGERCONTROLLER_H

class Window;
class Core;
class CoreController;

class DebuggerController {
public:
    DebuggerController(Window& window, Core& core, CoreController& core_controller);

    bool is_debugger_attached() const;
    bool attach_debugger(bool proceed_execution = false);
    bool detach_debugger();

private:
    Window& window;
    Core& core;
    CoreController& core_controller;
};

#endif // DEBUGGERCONTROLLER_H
