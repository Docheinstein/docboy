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
#ifndef ENABLE_CGB
    void on_prev_palette();
    void on_next_palette();
#endif

    struct {
#ifndef ENABLE_CGB
        MenuItem* palette {};
#endif
        MenuItem* scaling {};
    } items {};
};

#endif // GRAPHICSOPTIONSSCREEN_H
