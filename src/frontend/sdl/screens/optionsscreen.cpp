#include "screens/optionsscreen.h"

#include "controllers/navcontroller.h"
#include "screens/controloptionsscreen.h"
#include "screens/graphicsoptionsscreen.h"

OptionsScreen::OptionsScreen(Context ctx) :
    MenuScreen {ctx} {

    menu.add_item({"Graphics options", [this] {
                       nav.push(std::make_unique<GraphicsOptionsScreen>(context));
                   }});
    menu.add_item({"Control options", [this] {
                       nav.push(std::make_unique<ControlOptionsScreen>(context));
                   }});
    menu.add_item({"Back", [this] {
                       nav.pop();
                   }});
}