#ifndef SERIALOPTIONSSCREEN_H
#define SERIALOPTIONSSCREEN_H

#include "components/menu/menu.h"
#include "screens/menuscreen.h"
#include "screens/screen.h"

#include "docboy/joypad/joypad.h"

class CoreController;

class SerialOptionsScreen : public MenuScreen {
public:
    explicit SerialOptionsScreen(Context context);

    void redraw() override;

private:
    void on_serial_change();

    struct {
        MenuItem* serial;
    } items {};
};

#endif // CONTROLOPTIONSSCREEN_H
