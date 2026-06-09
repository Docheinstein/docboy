#include "screens/acceloptionsscreen.h"

#include "components/menu/items.h"

#include "controllers/maincontroller.h"
#include "controllers/navcontroller.h"

AccelerometerOptionsScreen::AccelerometerOptionsScreen(Context context) :
    MenuScreen {context} {

    items.accelerometer = &menu.add_item(Spinner {[this] {
                                                      on_accelerometer_sensitivity_decrease();
                                                  },
                                                  [this] {
                                                      on_accelerometer_sensitivity_increase();
                                                  }});

    menu.add_item(Button {"Back", [this] {
                              nav.pop();
                          }});
}

void AccelerometerOptionsScreen::redraw() {
    items.accelerometer->text = "Sensitivity    " + std::to_string(main.get_accelerometer_sensitivity());
    MenuScreen::redraw();
}

void AccelerometerOptionsScreen::on_accelerometer_sensitivity_increase() {
    main.set_accelerometer_sensitivity(std::min(main.get_accelerometer_sensitivity() + 1,
                                                static_cast<int>(MainController::ACCELEROMETER_SENSITIVITY_MAX)));
    redraw();
}

void AccelerometerOptionsScreen::on_accelerometer_sensitivity_decrease() {
    main.set_accelerometer_sensitivity(std::max(main.get_accelerometer_sensitivity() - 1,
                                                static_cast<int>(MainController::ACCELEROMETER_SENSITIVITY_MIN)));
    redraw();
}
