#include "screens/optionsscreen.h"

#include "components/menu/items.h"
#include "controllers/navcontroller.h"
#include "screens/controloptionsscreen.h"
#include "screens/graphicsoptionsscreen.h"

#ifdef ENABLE_AUDIO
#include "screens/audiooptionsscreen.h"
#endif

OptionsScreen::OptionsScreen(Context ctx) :
    MenuScreen {ctx} {

    menu.add_item(Button {"Graphics options", [this] {
                              nav.push(std::make_unique<GraphicsOptionsScreen>(context));
                          }});
#ifdef ENABLE_AUDIO
    menu.add_item(Button {"Audio options", [this] {
                              nav.push(std::make_unique<AudioOptionsScreen>(context));
                          }});
#endif
    menu.add_item(Button {"Control options", [this] {
                              nav.push(std::make_unique<ControlOptionsScreen>(context));
                          }});
    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
}