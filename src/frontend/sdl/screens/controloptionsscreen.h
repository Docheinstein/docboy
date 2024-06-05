#ifndef CONTROLOPTIONSSCREEN_H
#define CONTROLOPTIONSSCREEN_H

#include <optional>

#include "components/menu.h"
#include "screens/menuscreen.h"
#include "screens/screen.h"

#include "docboy/joypad/joypad.h"

class ControlOptionsScreen : public MenuScreen {
public:
    explicit ControlOptionsScreen(Context context);

    void redraw() override;
    void handle_event(const SDL_Event& event) override;

private:
    void on_remap_key(Joypad::Key key);

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

    std::optional<Joypad::Key> joypad_key_to_remap;
};

#endif // CONTROLOPTIONSSCREEN_H
