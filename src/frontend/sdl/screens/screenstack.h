#ifndef SCREENSTACK_H
#define SCREENSTACK_H

#include <functional>
#include <memory>
#include <vector>

class Screen;

struct ScreenStack {
    void push(std::unique_ptr<Screen> screen);
    void pop();

    std::vector<std::unique_ptr<Screen>> stack {};
    Screen* top {};

    std::function<void()> onScreenChanged {};
};

#endif // SCREENSTACK_H
