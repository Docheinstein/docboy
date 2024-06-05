#include "screens/shortcutsscreen.h"
#include "windows/window.h"

ShortcutsScreen::ShortcutsScreen(Context context) :
    MenuScreen {context} {

    menu.set_navigation_enabled(false);

    menu.add_item({"Toggle FPS      F"});
    menu.add_item({"Inc. speed      W"});
    menu.add_item({"Dec. speed      Q"});
#ifdef ENABLE_DEBUGGER
    menu.add_item({"Toggle debugger D"});
#endif
    menu.add_item({"Save state     F1"});
    menu.add_item({"Load state     F2"});
    menu.add_item({"Dump frame    F11"});
    menu.add_item({"Screenshot    F12"});
}