#include "screens/optionsscreen.h"

#include "components/menu/items.h"
#include "controllers/navcontroller.h"
#include "controllers/runcontroller.h"
#include "screens/controloptionsscreen.h"
#include "screens/graphicsoptionsscreen.h"
#include "screens/playerselectscreen.h"

#ifdef ENABLE_TWO_PLAYERS_MODE
#include "screens/serialoptionsscreen.h"
#endif

#ifdef ENABLE_AUDIO
#include "screens/audiooptionsscreen.h"
#endif

OptionsScreen::OptionsScreen(Context ctx) :
    MenuScreen {ctx} {

    menu.add_item(Button {"Graphics options", [this] {
                              nav.push(std::make_unique<GraphicsOptionsScreen>(context));
                          }});
#ifdef ENABLE_AUDIO
    menu.add_item(Button {"Audio options", [this] {
                              nav.push(std::make_unique<AudioOptionsScreen>(context));
                          }});
#endif
    menu.add_item(Button {"Control options", [this] {
                              if (runner.is_two_players_mode()) {
                                  nav.push(std::make_unique<PlayerSelectScreen>(
                                      context,
                                      [this]() {
                                          nav.push(std::make_unique<ControlOptionsScreen>(context, runner.get_core1()));
                                      },
                                      [this]() {
                                          nav.push(std::make_unique<ControlOptionsScreen>(context, runner.get_core2()));
                                      }));
                              } else {
                                  nav.push(std::make_unique<ControlOptionsScreen>(context, runner.get_core1()));
                              }
                          }});
#ifdef ENABLE_TWO_PLAYERS_MODE
    if (runner.is_two_players_mode()) {
        menu.add_item(Button {"Serial options", [this] {
                                  nav.push(std::make_unique<SerialOptionsScreen>(context));
                              }});
    }
#endif
    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
}