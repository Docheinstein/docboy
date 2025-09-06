#include "screens/gamemainscreen.h"

#include "components/menu/items.h"
#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/navcontroller.h"
#include "controllers/runcontroller.h"
#include "screens/helpscreen.h"
#include "screens/optionsscreen.h"

#include "screens/playerselectscreen.h"

#include "utils/rompicker.h"

GameMainScreen::GameMainScreen(Context ctx) :
    MenuScreen {ctx} {

    ASSERT(runner.get_core1().is_rom_loaded());

    menu.add_item(Button {"Save state", [this] {
#ifdef ENABLE_TWO_PLAYERS_MODE
                              if (runner.is_two_players_mode()) {
                                  nav.push(std::make_unique<PlayerSelectScreen>(
                                      context,
                                      [this]() {
                                          runner.get_core1().write_state();
                                          nav.pop();
                                      },
                                      [this]() {
                                          runner.get_core2().write_state();
                                          nav.pop();
                                      }));
                              } else
#endif
                              {
                                  runner.get_core1().write_state();
                                  nav.pop();
                              }
                          }});
    menu.add_item(Button {"Load state", [this] {
#ifdef ENABLE_TWO_PLAYERS_MODE
                              if (runner.is_two_players_mode()) {
                                  nav.push(std::make_unique<PlayerSelectScreen>(
                                      context,
                                      [this]() {
                                          runner.get_core1().load_state();
                                          nav.pop();
                                      },
                                      [this]() {
                                          runner.get_core2().load_state();
                                          nav.pop();
                                      }));
                              } else
#endif
                              {
                                  runner.get_core1().load_state();
                                  nav.pop();
                              }
                          }});

#ifdef NFD
    menu.add_item(Button {"Open ROM", [this] {
#ifdef ENABLE_TWO_PLAYERS_MODE
                              if (runner.is_two_players_mode()) {
                                  nav.push(std::make_unique<PlayerSelectScreen>(
                                      context,
                                      [this]() {
                                          if (const auto rom = open_rom_picker()) {
                                              runner.get_core1().load_rom(*rom);
                                              nav.pop();
                                          }
                                      },
                                      [this]() {
                                          if (const auto rom = open_rom_picker()) {
                                              runner.get_core2().load_rom(*rom);
                                              nav.pop();
                                          }
                                      }));
                              } else
#endif
                              {
                                  if (const auto rom = open_rom_picker()) {
                                      runner.get_core1().load_rom(*rom);
                                      nav.pop();
                                  }
                              }
                          }});
#endif

    menu.add_item(Button {"Options", [this] {
                              nav.push(std::make_unique<OptionsScreen>(context));
                          }});
    menu.add_item(Button {"Help", [this] {
                              nav.push(std::make_unique<HelpScreen>(context));
                          }});
    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
    menu.add_item(Button {"Exit", [this] {
                              main.quit();
                          }});
}
