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

    menu.add_item(MenuItem {"Save state"}.on_enter([this] {
        core.write_state();
        nav.pop();
    }));
    menu.add_item(MenuItem {"Load state"}.on_enter([this] {
        core.load_state();
        nav.pop();
    }));

#ifdef NFD
    menu.add_item(MenuItem {"Open ROM"}.on_enter([this] {
        if (const auto rom = open_rom_picker()) {
            core.load_rom(*rom);
            nav.pop();
        }
    }));
#endif

    menu.add_item(MenuItem {"Options"}.on_enter([this] {
        nav.push(std::make_unique<OptionsScreen>(context));
    }));
    menu.add_item(MenuItem {"Help"}.on_enter([this] {
        nav.push(std::make_unique<HelpScreen>(context));
    }));
    menu.add_item(MenuItem {"Back"}.on_enter([this] {
        nav.pop();
    }));
    menu.add_item(MenuItem {"Exit"}.on_enter([this] {
        main.quit();
    }));
}
