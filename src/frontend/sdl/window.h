#ifndef WINDOW_H
#define WINDOW_H

#include "core/ppu/lcd.h"
#include "core/definitions.h"
#include <chrono>
#include "sdllcd.h"

class SDL_Window;
class SDL_Renderer;
class SDL_Texture;

class Window {
public:
    explicit Window(SDLLCD &lcd, float scaling = 1.0);
    ~Window();

    void render();

private:
    SDLLCD &lcd;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    int width, height;

    std::chrono::high_resolution_clock::time_point lastRender;
};

#endif // WINDOW_H