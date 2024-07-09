#include "screens/optionsscreen.h"

#include "controllers/navcontroller.h"
#include "screens/controloptionsscreen.h"
#include "screens/graphicsoptionsscreen.h"

#ifdef ENABLE_AUDIO
#include "screens/audiooptionsscreen.h"
#endif

OptionsScreen::OptionsScreen(Context ctx) :
    MenuScreen {ctx} {

    menu.add_item(MenuItem {"Graphics options"}.on_enter([this] {
        nav.push(std::make_unique<GraphicsOptionsScreen>(context));
    }));
#ifdef ENABLE_AUDIO
    menu.add_item(MenuItem {"Audio options"}.on_enter([this] {
        nav.push(std::make_unique<AudioOptionsScreen>(context));
    }));
#endif
    menu.add_item(MenuItem {"Control options"}.on_enter([this] {
        nav.push(std::make_unique<ControlOptionsScreen>(context));
    }));
    menu.add_item(MenuItem {"Back"}.on_enter([this] {
        nav.pop();
    }));
}