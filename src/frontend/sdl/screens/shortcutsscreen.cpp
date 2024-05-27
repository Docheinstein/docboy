#include "shortcutsscreen.h"
#include "window.h"

ShortcutsScreen::ShortcutsScreen(Context context) :
    MenuScreen(context) {

    menu.setNavigationEnabled(false);

    menu.addItem({"Toggle FPS      F"});
    menu.addItem({"Inc. speed      W"});
    menu.addItem({"Dec. speed      Q"});
#ifdef ENABLE_DEBUGGER
    menu.addItem({"Toggle debugger D"});
#endif
    menu.addItem({"Save state     F1"});
    menu.addItem({"Load state     F2"});
    menu.addItem({"Dump frame    F11"});
    menu.addItem({"Screenshot    F12"});
}