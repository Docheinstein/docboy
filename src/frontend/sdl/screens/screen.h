#ifndef SCREEN_H
#define SCREEN_H

#include "docboy/shared/macros.h"
#include <memory>

class CoreController;
class UiController;
class NavController;
class MainController;

#ifdef ENABLE_DEBUGGER
class DebuggerController;
#endif

struct SDL_Renderer;
union SDL_Event;

class Screen {
public:
    struct Controllers {
        CoreController& core;
        UiController& ui;
        NavController& nav;
        MainController& main;
#ifdef ENABLE_DEBUGGER
        DebuggerController& debugger;
#endif
    };

    struct Context {
        Controllers controllers;
        struct {
            uint8_t backgroundAlpha;
        } ui;
    };

    explicit Screen(Context context);

    virtual ~Screen() = default;

    virtual void redraw() = 0;
    virtual void render() = 0;
    virtual void handleEvent(const SDL_Event& event) = 0;

protected:
    Context context;

    CoreController& core;
    UiController& ui;
    NavController& nav;
    MainController& main;
#ifdef ENABLE_DEBUGGER
    DebuggerController& debugger;
#endif

    SDL_Renderer* renderer {};
};

#endif // SCREEN_H
