#include "window.h"
#include "helpers.h"
#include "utils/binutils.h"
#include <SDL.h>
#include <stdexcept>

Window::~Window() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

Window::Window(uint32_t* framebuffer, ILCDIO& lcd, int x, int y, float scaling) :
    framebuffer(framebuffer),
    lcd(lcd),
    window(),
    renderer(),
    texture(),
    width(static_cast<int>(scaling * Specs::Display::WIDTH)),
    height(static_cast<int>(scaling * Specs::Display::HEIGHT)) {

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init error: ") + SDL_GetError());

    window =
        SDL_CreateWindow("DocBoy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (!window)
        throw std::runtime_error(std::string("SDL_CreateWindow error: ") + SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        throw std::runtime_error(std::string("SDL_CreateRenderer error: ") + SDL_GetError());

    // TODO: figure out byte order
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, Specs::Display::WIDTH,
                                Specs::Display::HEIGHT);
    if (!texture)
        throw std::runtime_error(std::string("SDL_CreateTexture error: ") + SDL_GetError());

    // TODO: don't know but setting these in SDL_CreateWindow does not work
    const auto winX = static_cast<int>(x == POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : x);
    const auto winY = static_cast<int>(y == POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : y);
    SDL_SetWindowPosition(window, winX, winY);
}

void Window::render() {
    auto now = std::chrono::high_resolution_clock::now();
    if (now < lastRender + std::chrono::milliseconds(16)) {
        return;
    }
    lastRender = now;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    bool lcdOn = get_bit<Bits::LCD::LCDC::LCD_ENABLE>(lcd.readLCDC());
    if (lcdOn) {
        SDL_Rect srcrect {.x = 0, .y = 0, .w = Specs::Display::WIDTH, .h = Specs::Display::HEIGHT};
        SDL_Rect dstrect {.x = 0, .y = 0, .w = width, .h = height};
        void* texturePixels;
        int pitch;

        SDL_LockTexture(texture, &srcrect, (void**)&texturePixels, &pitch);
        memcpy(texturePixels, framebuffer, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint32_t));
        SDL_UnlockTexture(texture);

        SDL_RenderCopy(renderer, texture, nullptr, &dstrect);
    }

    SDL_RenderPresent(renderer);
}

std::pair<int, int> Window::getPosition() const {
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return {x, y};
}
