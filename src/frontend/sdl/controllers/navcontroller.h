#ifndef NAVCONTROLLER_H
#define NAVCONTROLLER_H

#include <memory>

class ScreenStack;
class Screen;

class NavController {
public:
    explicit NavController(ScreenStack& screen_stack);

    void push(std::unique_ptr<Screen> screen);
    void pop();

protected:
    ScreenStack& screen_stack;
};

#endif // NAVCONTROLLER_H
