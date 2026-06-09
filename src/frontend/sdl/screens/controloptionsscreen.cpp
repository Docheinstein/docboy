#include "screens/controloptionsscreen.h"

#include "screens/acceloptionsscreen.h"
#include "screens/keyboardcontroloptionsscreen.h"
#include "screens/playerselectscreen.h"

#include "controllers/navcontroller.h"
#include "controllers/runcontroller.h"

#include "components/menu/items.h"

ControlOptionsScreen::ControlOptionsScreen(Context ctx) :
    MenuScreen(ctx) {

    menu.add_item(
        Button {"Keyboard options", [this] {
#ifdef ENABLE_TWO_PLAYERS_MODE
                    if (runner.is_two_players_mode()) {
                        nav.push(std::make_unique<PlayerSelectScreen>(
                            context,
                            [this]() {
                                nav.push(std::make_unique<KeyboardControlOptionsScreen>(context, runner.get_core1()));
                            },
                            [this]() {
                                nav.push(std::make_unique<KeyboardControlOptionsScreen>(context, runner.get_core2()));
                            }));
                    } else
#endif
                    {
                        nav.push(std::make_unique<KeyboardControlOptionsScreen>(context, runner.get_core1()));
                    }
                }});

    menu.add_item(Button {"Accel. options", [this] {
                              nav.push(std::make_unique<AccelerometerOptionsScreen>(context));
                          }});

    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
}