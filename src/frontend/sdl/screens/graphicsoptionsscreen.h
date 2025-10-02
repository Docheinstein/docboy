#ifndef GRAPHICSOPTIONSSCREEN_H
#define GRAPHICSOPTIONSSCREEN_H

#include "screens/menuscreen.h"

class GraphicsOptionsScreen : public MenuScreen {
public:
    explicit GraphicsOptionsScreen(Context context);

    void redraw() override;

private:
    void on_decrease_scaling();
    void on_increase_scaling();
#ifndef ENABLE_CGB
    void on_prev_palette();
    void on_next_palette();
#endif

    void on_prev_scaling_filter();
    void on_next_scaling_filter();

    struct {
#ifndef ENABLE_CGB
        MenuItem* palette {};
#endif
        MenuItem* scaling {};
        MenuItem* scaling_filter_label {};
        MenuItem* scaling_filter {};
    } items {};
};

#endif // GRAPHICSOPTIONSSCREEN_H
