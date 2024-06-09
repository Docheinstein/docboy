#include "screens/gamemainscreen.h"

#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/navcontroller.h"
#include "screens/helpscreen.h"
#include "screens/optionsscreen.h"
#include "utils/rompicker.h"

GameMainScreen::GameMainScreen(Context ctx) :
    MenuScreen {ctx} {

    ASSERT(core.is_rom_loaded());

    menu.add_item({"Save state", [this] {
                       core.write_state();
                       nav.pop();
                   }});
    menu.add_item({"Load state", [this] {
                       core.load_state();
                       nav.pop();
                   }});

#ifdef NFD
    menu.add_item({"Open ROM", [this] {
                       if (const auto rom = open_rom_picker()) {
                           core.load_rom(*rom);
                           nav.pop();
                       }
                   }});
#endif

    menu.add_item({"Options", [this] {
                       nav.push(std::make_unique<OptionsScreen>(context));
                   }});
    menu.add_item({"Help", [this] {
                       nav.push(std::make_unique<HelpScreen>(context));
                   }});
    menu.add_item({"Back", [this] {
                       nav.pop();
                   }});
    menu.add_item({"Exit", [this] {
                       main.quit();
                   }});
}
