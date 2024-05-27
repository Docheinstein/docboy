#ifndef CONTROLOPTIONSSCREEN_H
#define CONTROLOPTIONSSCREEN_H

#include "components/menu.h"
#include "docboy/joypad/joypad.h"
#include "menuscreen.h"
#include "screen.h"
#include <optional>

class ControlOptionsScreen : public MenuScreen {
public:
    explicit ControlOptionsScreen(Context context);

    void redraw() override;
    void handleEvent(const SDL_Event& event) override;

private:
    void onRemapKey(Joypad::Key key);

    struct {
        Menu::MenuItem* a;
        Menu::MenuItem* b;
        Menu::MenuItem* start;
        Menu::MenuItem* select;
        Menu::MenuItem* left;
        Menu::MenuItem* up;
        Menu::MenuItem* right;
        Menu::MenuItem* down;
    } items {};

    std::optional<Joypad::Key> joypadKeyToRemap;
};

#endif // CONTROLOPTIONSSCREEN_H
