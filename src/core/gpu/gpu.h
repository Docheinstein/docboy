#ifndef GPU_H
#define GPU_H

#include "core/vram.h"
#include "core/io.h"
#include "core/display.h"

class IGPU {
public:
    virtual ~IGPU() = default;
    virtual void tick() = 0;
};

class GPU : public IGPU {
public:
    GPU(IDisplay &display, VRAM &vram, IO &io);

    void tick() override;

private:
    IDisplay &display;
    VRAM &vram;
    IO &io;

    IDisplay::Pixel pixels[Specs::Display::WIDTH * Specs::Display::HEIGHT];
};

#endif // GPU_H