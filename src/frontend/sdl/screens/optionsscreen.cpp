#include "optionsscreen.h"
#include "controllers/navcontroller.h"
#include "controloptionsscreen.h"
#include "graphicsoptionsscreen.h"

OptionsScreen::OptionsScreen(Context ctx) :
    MenuScreen(ctx) {

    menu.addItem({"Graphics options", [this] {
                      nav.push(std::make_unique<GraphicsOptionsScreen>(context));
                  }});
    menu.addItem({"Control options", [this] {
                      nav.push(std::make_unique<ControlOptionsScreen>(context));
                  }});
    menu.addItem({"Back", [this] {
                      nav.pop();
                  }});
}