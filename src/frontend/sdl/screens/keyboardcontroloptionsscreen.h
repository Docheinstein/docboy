#ifndef KEYBOARDCONTROLOPTIONSSCREEN_H
#define KEYBOARDCONTROLOPTIONSSCREEN_H

#include <optional>

#include "components/menu/menu.h"

#include "screens/menuscreen.h"
#include "screens/screen.h"

#include "docboy/joypad/joypad.h"

class CoreController;

class KeyboardControlOptionsScreen : public MenuScreen {
public:
    explicit KeyboardControlOptionsScreen(Context context, CoreController& core);

    void redraw() override;
    void handle_event(const SDL_Event& event) override;

private:
    void on_remap_key(Joypad::Key key);

    CoreController& core;

    struct {
        MenuItem* a;
        MenuItem* b;
        MenuItem* start;
        MenuItem* select;
        MenuItem* left;
        MenuItem* up;
        MenuItem* right;
        MenuItem* down;
    } items {};

    std::optional<Joypad::Key> joypad_key_to_remap;
};

#endif // KEYBOARDCONTROLOPTIONSSCREEN_H
