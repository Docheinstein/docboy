#ifndef PLAYERSELECTSCREEN_H
#define PLAYERSELECTSCREEN_H

#include "screens/menuscreen.h"

class PlayerSelectScreen : public MenuScreen {
public:
    PlayerSelectScreen(Context context, std::function<void()>&& p1_callback, std::function<void()>&& p2_callback);

private:
    std::function<void()> player1_callback {};
    std::function<void()> player2_callback {};
};

#endif // PLAYERSELECTSCREEN_H
