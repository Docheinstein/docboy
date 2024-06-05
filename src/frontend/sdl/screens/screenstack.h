#ifndef SCREENSTACK_H
#define SCREENSTACK_H

#include <functional>
#include <memory>
#include <vector>

class Screen;

class ScreenStack {
public:
    void push(std::unique_ptr<Screen> screen);
    void pop();

    void set_on_screen_changed_callback(std::function<void()>&& callback);

    std::vector<std::unique_ptr<Screen>> stack {};
    Screen* top {};

private:
    std::function<void()> on_screen_changed {};
};

#endif // SCREENSTACK_H
