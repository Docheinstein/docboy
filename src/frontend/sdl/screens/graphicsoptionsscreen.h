#ifndef GRAPHICSOPTIONSSCREEN_H
#define GRAPHICSOPTIONSSCREEN_H

#include "menuscreen.h"

class GraphicsOptionsScreen : public MenuScreen {
public:
    explicit GraphicsOptionsScreen(Context context);

    void redraw() override;

private:
    void onIncreaseScaling();
    void onDecreaseScaling();
    void onPrevPalette();
    void onNextPalette();

    Menu::MenuItem* paletteItem;
    Menu::MenuItem* scalingItem;
};

#endif // GRAPHICSOPTIONSSCREEN_H
