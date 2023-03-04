#ifndef NOGUIDISPLAY_H
#define NOGUIDISPLAY_H

#include "core/display.h"

class NoGuiDisplay : public IDisplay {
public:
    NoGuiDisplay() = default;
    void render(Pixel *pixels) override;
};

#endif // NOGUIDISPLAY_H