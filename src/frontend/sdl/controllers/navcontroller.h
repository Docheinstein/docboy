#ifndef NAVCONTROLLER_H
#define NAVCONTROLLER_H

#include <memory>

struct ScreenStack;
class Screen;

class NavController {
public:
    explicit NavController(ScreenStack& screenStack);

    void push(std::unique_ptr<Screen> screen);
    void pop();

protected:
    ScreenStack& screenStack;
};

#endif // NAVCONTROLLER_H
