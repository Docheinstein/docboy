#ifndef WINDOW_H
#define WINDOW_H

#include "core/display.h"
#include <chrono>

class SDL_Window;
class SDL_Renderer;
class SDL_Surface;
class SDL_Texture;

class Window : public IDisplay {
public:
    Window();
    ~Window() override;

    void show();

    void render(Pixel *pixels) override;

private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    // DEBUG
    std::chrono::high_resolution_clock::time_point lastRender;
};

#endif // WINDOW_H