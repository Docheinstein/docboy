#include "helpscreen.h"
#include "controllers/navcontroller.h"
#include "shortcutsscreen.h"

HelpScreen::HelpScreen(Context ctx) :
    MenuScreen(ctx) {

    menu.addItem({"Shortcuts", [this] {
                      nav.push(std::make_unique<ShortcutsScreen>(context));
                  }});
    menu.addItem({"Back", [this] {
                      nav.pop();
                  }});
}
