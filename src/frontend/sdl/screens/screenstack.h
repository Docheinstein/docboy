#ifndef SCREENSTACK_H
#define SCREENSTACK_H

#include <memory>
#include <vector>

class Screen;

struct ScreenStack {
    void push(std::unique_ptr<Screen> screen);
    void pop();

    std::vector<std::unique_ptr<Screen>> stack {};
    Screen* top {};
};

#endif // SCREENSTACK_H
