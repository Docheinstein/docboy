#include "screens/mainscreen.h"

#include "components/menu/items.h"
#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/runcontroller.h"
#include "screens/gamescreen.h"
#include "screens/helpscreen.h"
#include "screens/optionsscreen.h"

#include "utils/rompicker.h"

MainScreen::MainScreen(Context ctx) :
    MenuScreen {ctx} {

#ifdef NFD
    menu.add_item(Button {"Open ROM", [this] {
                              if (const auto rom = open_rom_picker()) {
                                  runner.get_core1().load_rom(*rom);
                                  nav.push(std::make_unique<GameScreen>(context));
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