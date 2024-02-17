#include "window.h"
#include "docboy/shared/specs.h"
#include "glyphs.h"
#include "utils/exceptions.hpp"
#include "utils/log.h"
#include "utils/std.hpp"
#include <SDL.h>
#include <string>

#ifdef SDL2_IMAGE
#include <SDL_image.h>
#include <chrono>

#endif

Window::Window(const uint16_t* framebuffer, int x, int y, float scaling) :
    framebuffer(framebuffer),
    width(static_cast<int>(scaling * Specs::Display::WIDTH)),
    height(static_cast<int>(scaling * Specs::Display::HEIGHT)),
    scaling(scaling) {

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        fatal("SDL_Init error: " + SDL_GetError());

    const int winX = static_cast<int>(x == WINDOW_POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : x);
    const int winY = static_cast<int>(y == WINDOW_POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : y);

    window = SDL_CreateWindow("DocBoy", winX, winY, width, height, SDL_WINDOW_SHOWN);
    if (!window)
        fatal("SDL_CreateWindow error: " + SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        fatal("SDL_CreateRenderer error: " + SDL_GetError());

    // TODO: figure out byte order
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, Specs::Display::WIDTH,
                                Specs::Display::HEIGHT);
    if (!texture)
        fatal("SDL_CreateTexture error: " + SDL_GetError());

    // TODO: don't know but setting these in SDL_CreateWindow does not work
    //    SDL_SetWindowPosition(window, winX, winY);
}

Window::~Window() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    drawFramebuffer();
    drawTexts();

    SDL_RenderPresent(renderer);
}

void Window::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

uint64_t Window::addText(const std::string& text, int x, int y, uint32_t color, std::optional<uint64_t> millisAlive,
                         std::optional<uint64_t> optionalGuid) {
    const uint64_t guid = optionalGuid ? *optionalGuid : rand(); // NOLINT(cert-msc50-cpp)
    removeText(guid);

    texts.push_back({guid, text, x, y, color,
                     millisAlive ? std::optional(std::chrono::high_resolution_clock::now() +
                                                 std::chrono::milliseconds(*millisAlive))
                                 : std::nullopt});
    return guid;
}

void Window::removeText(uint64_t guid) {
    erase_if(texts, [guid](const Text& t) {
        return t.guid == guid;
    });
}

void Window::drawFramebuffer() {
    SDL_Rect srcrect {0, 0, Specs::Display::WIDTH, Specs::Display::HEIGHT};
    SDL_Rect dstrect {0, 0, width, height};

    void* texturePixels;
    int pitch;

    SDL_LockTexture(texture, &srcrect, (void**)&texturePixels, &pitch);
    memcpy(texturePixels, framebuffer, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t));
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, nullptr, &dstrect);
}

void Window::drawTexts() {
    const auto now = std::chrono::high_resolution_clock::now();

    auto it = texts.begin();
    while (it != texts.end()) {
        Text& text = *it;
        drawText(text);
        if (text.deadline && now > *text.deadline)
            it = texts.erase(it);
        else
            it++;
    }
}

void Window::drawText(const Window::Text& text) {
    const uint32_t stringLength = text.string.size();
    const int textWidth = static_cast<int>(GLYPH_WIDTH * stringLength);
    const int textHeight = GLYPH_HEIGHT;

    SDL_Texture* textTexture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textWidth, textHeight);
    SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);

    uint32_t* textureBuffer;
    int pitch;

    SDL_LockTexture(textTexture, nullptr, (void**)&textureBuffer, &pitch);
    for (uint32_t i = 0; i < stringLength; i++)
        glyph_to_rgba(char_to_glyph(text.string[i]), text.color, textureBuffer, i, stringLength);
    SDL_UnlockTexture(textTexture);

    SDL_Rect textDstRect {
        static_cast<int>(scaling * static_cast<float>(text.x)),
        static_cast<int>(scaling * static_cast<float>(text.y)),
        static_cast<int>(scaling * static_cast<float>(textWidth)),
        static_cast<int>(scaling * static_cast<float>(textHeight)),
    };

    SDL_RenderCopy(renderer, textTexture, nullptr, &textDstRect);
    SDL_DestroyTexture(textTexture);
}

bool Window::screenshot(const std::string& filename) const {
#ifdef SDL2_IMAGE
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        (void*)framebuffer, Specs::Display::WIDTH, Specs::Display::HEIGHT, SDL_BITSPERPIXEL(SDL_PIXELFORMAT_RGB565),
        Specs::Display::WIDTH * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGB565), SDL_PIXELFORMAT_RGB565);

    if (!surface)
        fatal("SDL_CreateRGBSurfaceWithFormatFrom error: " + SDL_GetError());

    int result = IMG_SavePNG(surface, filename.c_str());

    SDL_FreeSurface(surface);

    if (result != 0)
        WARN("SDL_SaveBMP error: " + SDL_GetError());

    return result == 0;
#else
    WARN("Unsupported");
    return false;
#endif
}
