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
    menu.add_item(MenuItem {"Open ROM"}.on_enter([this] {
        if (const auto rom = open_rom_picker()) {
            core.load_rom(*rom);
            nav.push(std::make_unique<GameScreen>(context));
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