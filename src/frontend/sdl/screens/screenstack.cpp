#include "screens/screenstack.h"

#include "screens/screen.h"

void ScreenStack::push(std::unique_ptr<Screen> screen) {
    stack.emplace_back(std::move(screen));
    top = &*stack.back();
    if (on_screen_changed) {
        on_screen_changed();
    }
}

void ScreenStack::pop() {
    stack.pop_back();
    top = !stack.empty() ? &*stack.back() : nullptr;
    if (on_screen_changed) {
        on_screen_changed();
    }
}

void ScreenStack::set_on_screen_changed_callback(std::function<void()>&& callback) {
    on_screen_changed = std::move(callback);
}
