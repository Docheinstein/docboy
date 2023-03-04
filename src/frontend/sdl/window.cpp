#include "window.h"
#include <SDL.h>
#include <stdexcept>
#include "core/definitions.h"

Window::Window() {

}

Window::~Window() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window::show() {
    // TODO: scaling
    int width = Specs::Display::WIDTH;
    int height = Specs::Display::HEIGHT;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init error: ") + SDL_GetError());

    window = SDL_CreateWindow(
            "DocBoy",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            width, height,
            SDL_WINDOW_SHOWN);
    if (!window)
        throw std::runtime_error(std::string("SDL_CreateWindow error: ") + SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        throw std::runtime_error(std::string("SDL_CreateRenderer error: ") + SDL_GetError());

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture)
        throw std::runtime_error(std::string("SDL_CreateTexture error: ") + SDL_GetError());
}

void Window::render(Pixel *pixels) {
    auto now = std::chrono::high_resolution_clock::now();
    if (now - lastRender < std::chrono::milliseconds(16)) {
        return;
    }
    lastRender = now;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Rect rect { .x = 0, .y = 0, .w = Specs::Display::WIDTH, .h = Specs::Display::HEIGHT };
    void *texturePixels;
    int pitch;

    SDL_LockTexture(texture, &rect, (void **) &texturePixels, &pitch);
    memcpy(texturePixels, pixels, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(Pixel));
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}


