#ifndef ACCELEROMETEROPTIONSSCREEN_H
#define ACCELEROMETEROPTIONSSCREEN_H

#include "screens/menuscreen.h"

class CoreController;

class AccelerometerOptionsScreen : public MenuScreen {
public:
    explicit AccelerometerOptionsScreen(Context context);

    void redraw() override;

private:
    void on_accelerometer_sensitivity_increase();
    void on_accelerometer_sensitivity_decrease();

    struct {
        MenuItem* accelerometer;
    } items {};
};

#endif // ACCELEROMETEROPTIONSSCREEN_H
