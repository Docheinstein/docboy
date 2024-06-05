#include "controllers/navcontroller.h"

#include "screens/screen.h"
#include "screens/screenstack.h"

NavController::NavController(ScreenStack& screen_stack) :
    screen_stack {screen_stack} {
}

void NavController::push(std::unique_ptr<Screen> screen) {
    screen_stack.push(std::move(screen));
    if (screen_stack.top) {
        screen_stack.top->redraw();
    }
}

void NavController::pop() {
    screen_stack.pop();
    if (screen_stack.top) {
        screen_stack.top->redraw();
    }
}
