#include "screens/gamemainscreen.h"

#include "components/menu/items.h"
#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/navcontroller.h"
#include "screens/helpscreen.h"
#include "screens/optionsscreen.h"

#include "utils/rompicker.h"

GameMainScreen::GameMainScreen(Context ctx) :
    MenuScreen {ctx} {

    ASSERT(core.is_rom_loaded());

    menu.add_item(Button {"Save state", [this] {
                              core.write_state();
                              nav.pop();
                          }});
    menu.add_item(Button {"Load state", [this] {
                              core.load_state();
                              nav.pop();
                          }});

#ifdef NFD
    menu.add_item(Button {"Open ROM", [this] {
                              if (const auto rom = open_rom_picker()) {
                                  core.load_rom(*rom);
                                  nav.pop();
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
