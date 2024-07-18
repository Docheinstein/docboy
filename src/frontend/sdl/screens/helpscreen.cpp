#include "screens/helpscreen.h"

#include "components/menu/items.h"
#include "controllers/navcontroller.h"
#include "screens/shortcutsscreen.h"

HelpScreen::HelpScreen(Context ctx) :
    MenuScreen {ctx} {

    menu.add_item(Button {"Shortcuts", [this] {
                              nav.push(std::make_unique<ShortcutsScreen>(context));
                          }});
    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
}
