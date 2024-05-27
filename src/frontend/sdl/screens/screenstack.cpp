#include "screenstack.h"
#include "screens/screen.h"

void ScreenStack::push(std::unique_ptr<Screen> screen) {
    stack.emplace_back(std::move(screen));
    top = &*stack.back();
}

void ScreenStack::pop() {
    stack.pop_back();
    top = !stack.empty() ? &*stack.back() : nullptr;
}
