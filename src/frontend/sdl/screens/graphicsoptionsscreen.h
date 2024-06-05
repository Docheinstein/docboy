#ifndef GRAPHICSOPTIONSSCREEN_H
#define GRAPHICSOPTIONSSCREEN_H

#include "screens/menuscreen.h"

class GraphicsOptionsScreen : public MenuScreen {
public:
    explicit GraphicsOptionsScreen(Context context);

    void redraw() override;

private:
    void on_increase_scaling();
    void on_decrease_scaling();
    void on_prev_palette();
    void on_next_palette();

    struct {
        Menu::MenuItem* palette {};
        Menu::MenuItem* scaling {};
    } items {};
};

#endif // GRAPHICSOPTIONSSCREEN_H
