#include "screens/screen.h"

#include "controllers/uicontroller.h"

Screen::Screen(Context ctx) :
    context {ctx},
    runner {ctx.controllers.runner},
    ui {ctx.controllers.ui},
    nav {ctx.controllers.nav},
    main {ctx.controllers.main},
#ifdef ENABLE_DEBUGGER
    debugger {ctx.controllers.debugger},
#endif
    renderer {ctx.controllers.ui.get_renderer()} {
}