#include "screen.h"
#include "controllers/uicontroller.h"

Screen::Screen(Context ctx) :
    context(ctx),
    core(ctx.controllers.core),
    ui(ctx.controllers.ui),
    nav(ctx.controllers.nav),
    main(ctx.controllers.main),
#ifdef ENABLE_DEBUGGER
    debugger(ctx.controllers.debugger),
#endif
    renderer(ctx.controllers.ui.getRenderer()) {
}