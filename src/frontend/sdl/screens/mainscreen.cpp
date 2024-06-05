#include "screens/mainscreen.h"

#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "screens/gamescreen.h"
#include "screens/helpscreen.h"
#include "screens/optionsscreen.h"
#include "utils/rompicker.h"

MainScreen::MainScreen(Context ctx) :
    MenuScreen {ctx} {

#ifdef NFD
    menu.add_item({"Load ROM", [this] {
                       if (const auto rom = open_rom_picker()) {
                           core.load_rom(*rom);
                           core.set_paused(false);
                           nav.push(std::make_unique<GameScreen>(context));
                       }
                   }});
#endif

    menu.add_item({"Options", [this] {
                       nav.push(std::make_unique<OptionsScreen>(context));
                   }});
    menu.add_item({"Help", [this] {
                       nav.push(std::make_unique<HelpScreen>(context));
                   }});
    menu.add_item({"Exit", [this] {
                       main.quit();
                   }});
}