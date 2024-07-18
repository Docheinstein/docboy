#include "screens/shortcutsscreen.h"
#include "components/menu/items.h"
#include "windows/window.h"

ShortcutsScreen::ShortcutsScreen(Context context) :
    MenuScreen {context} {

    menu.add_item(MenuItem {"Toggle FPS      F"});
    menu.add_item(MenuItem {"Inc. speed      W"});
    menu.add_item(MenuItem {"Dec. speed      Q"});
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
    menu.add_item(MenuItem {"Dump frame    F11"});
    menu.add_item(MenuItem {"Screenshot    F12"});
}