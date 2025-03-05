#ifndef CONTROLOPTIONSSCREEN_H
#define CONTROLOPTIONSSCREEN_H

#include <optional>

#include "components/menu/menu.h"
#include "screens/menuscreen.h"
#include "screens/screen.h"

#include "docboy/joypad/joypad.h"

class CoreController;

class ControlOptionsScreen : public MenuScreen {
public:
    explicit ControlOptionsScreen(Context context, CoreController& core);

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

#endif // CONTROLOPTIONSSCREEN_H
