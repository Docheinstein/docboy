#include "screens/serialoptionsscreen.h"

#include "controllers/navcontroller.h"
#include "controllers/runcontroller.h"

#include "components/menu/items.h"

SerialOptionsScreen::SerialOptionsScreen(Context context) :
    MenuScreen(context) {

    items.serial = &menu.add_item(Spinner {[this] {
        on_serial_change();
    }});

    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
}

void SerialOptionsScreen::redraw() {
    items.serial->text = "Link  " + std::string(runner.is_serial_link_attached() ? "Attached" : "Detached");
    MenuScreen::redraw();
}

void SerialOptionsScreen::on_serial_change() {
    if (runner.is_serial_link_attached()) {
        runner.detach_serial_link();
    } else {
        runner.attach_serial_link();
    }
    redraw();
}
