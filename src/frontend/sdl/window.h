#ifndef WINDOW_H
#define WINDOW_H

#include "core/definitions.h"
#include "core/lcd/framebufferlcd.h"
#include <chrono>

class SDL_Window;
class SDL_Renderer;
class SDL_Texture;

class Window {
public:
    explicit Window(uint32_t* framebuffer, ILCDIO& lcd, float scaling = 1.0);
    ~Window();

    void render();

private:
    uint32_t* framebuffer;
    ILCDIO& lcd;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    int width, height;

    std::chrono::high_resolution_clock::time_point lastRender;
};

#endif // WINDOW_H