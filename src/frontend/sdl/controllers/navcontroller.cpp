#include "navcontroller.h"
#include "screens/screen.h"
#include "screens/screenstack.h"

NavController::NavController(ScreenStack& screenStack) :
    screenStack(screenStack) {
}

void NavController::push(std::unique_ptr<Screen> screen) {
    screenStack.push(std::move(screen));
    if (screenStack.top)
        screenStack.top->redraw();
}

void NavController::pop() {
    screenStack.pop();
    if (screenStack.top)
        screenStack.top->redraw();
}
