#include "mainscreen.h"
#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "gamescreen.h"
#include "helpscreen.h"
#include "optionsscreen.h"
#include "utils/rompicker.h"

MainScreen::MainScreen(Context ctx) :
    MenuScreen(ctx) {

#ifdef NFD
    menu.addItem({"Load ROM", [this] {
                      if (const auto rom = openRomPicker()) {
                          core.loadRom(*rom);
                          core.setPaused(false);
                          nav.push(std::make_unique<GameScreen>(context));
                      }
                  }});
#endif

    menu.addItem({"Options", [this] {
                      nav.push(std::make_unique<OptionsScreen>(context));
                  }});
    menu.addItem({"Help", [this] {
                      nav.push(std::make_unique<HelpScreen>(context));
                  }});
    menu.addItem({"Exit", [this] {
                      main.quit();
                  }});
}