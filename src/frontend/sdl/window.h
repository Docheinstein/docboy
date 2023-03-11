#ifndef WINDOW_H
#define WINDOW_H

#include <chrono>
#include "core/lcd/framebufferlcd.h"
#include "core/definitions.h"

class SDL_Window;
class SDL_Renderer;
class SDL_Texture;

class Window {
public:
    explicit Window(IFrameBufferLCD &lcd, float scaling = 1.0);
    ~Window();

    void render();
    bool screenshot(const char *filename);

private:
    IFrameBufferLCD &lcd;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    int width, height;

    std::chrono::high_resolution_clock::time_point lastRender;
};

#endif // WINDOW_H