#include "screens/helpscreen.h"

#include "controllers/navcontroller.h"
#include "screens/shortcutsscreen.h"

HelpScreen::HelpScreen(Context ctx) :
    MenuScreen {ctx} {

    menu.add_item({"Shortcuts", [this] {
                       nav.push(std::make_unique<ShortcutsScreen>(context));
                   }});
    menu.add_item({"Back", [this] {
                       nav.pop();
                   }});
}
