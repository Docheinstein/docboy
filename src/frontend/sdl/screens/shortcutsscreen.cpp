#include "screens/shortcutsscreen.h"

#include "controllers/runcontroller.h"

#include "components/menu/items.h"

#include "windows/window.h"

ShortcutsScreen::ShortcutsScreen(Context context) :
    MenuScreen {context} {

    menu.add_item(MenuItem {"Toggle FPS      F"});
    menu.add_item(MenuItem {"Inc. speed      W"});
    menu.add_item(MenuItem {"Dec. speed      Q"});
    menu.add_item(MenuItem {"Max. speed  Space"});
#ifdef ENABLE_AUDIO
    menu.add_item(MenuItem {"Toggle audio    M"});
#endif
    menu.add_item(MenuItem {"Toggle FPS      F"});
    menu.add_item(MenuItem {"Inc. speed      W"});
    menu.add_item(MenuItem {"Dec. speed      Q"});
#ifdef ENABLE_DEBUGGER
    menu.add_item(MenuItem {"Toggle debugger D"});
#endif
    menu.add_item(MenuItem {"Save state     F1"});
    menu.add_item(MenuItem {"Load state     F2"});
#ifdef ENABLE_TWO_PLAYERS_MODE
    if (runner.is_two_players_mode()) {
        menu.add_item(MenuItem {"Save state (2) F3"});
        menu.add_item(MenuItem {"Load state (2) F4"});
    }
#endif
    menu.add_item(MenuItem {"Dump frame    F11"});
    menu.add_item(MenuItem {"Screenshot    F12"});
}