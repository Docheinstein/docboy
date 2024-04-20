#include "window.h"
#include "docboy/shared/specs.h"
#include "extra/img/imgmanip.h"
#include "extra/png/iopng.h"
#include "glyphs.h"
#include "png.h"
#include "utils/exceptions.hpp"
#include "utils/std.hpp"
#include <SDL3/SDL.h>
#include <chrono>
#include <iostream>
#include <string>

Window::Window(const uint16_t* framebuffer, const Geometry& geometry) :
    framebuffer(framebuffer),
    width(geometry.scaling * Specs::Display::WIDTH),
    height(geometry.scaling * Specs::Display::HEIGHT),
    scaling(geometry.scaling) {

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        fatal("SDL_Init error: " + SDL_GetError());

    const int winX = static_cast<int>(geometry.x == WINDOW_POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : geometry.x);
    const int winY = static_cast<int>(geometry.y == WINDOW_POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : geometry.y);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "DocBoy");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, winX);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, winY);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    window = SDL_CreateWindowWithProperties(props);
    if (!window)
        fatal("SDL_CreateWindow error: " + SDL_GetError());

    renderer = SDL_CreateRenderer(window, nullptr, 0);
    if (!renderer)
        fatal("SDL_CreateRenderer error: " + SDL_GetError());

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, Specs::Display::WIDTH,
                                Specs::Display::HEIGHT);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    if (!texture)
        fatal("SDL_CreateTexture error: " + SDL_GetError());
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
    SDL_FRect dstrect {0, 0, width, height};

    void* texturePixels;
    int pitch;

    SDL_LockTexture(texture, &srcrect, (void**)&texturePixels, &pitch);
    memcpy(texturePixels, framebuffer, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t));
    SDL_UnlockTexture(texture);

    SDL_RenderTexture(renderer, texture, nullptr, &dstrect);
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
    SDL_SetTextureScaleMode(textTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);

    uint32_t* textureBuffer;
    int pitch;

    SDL_LockTexture(textTexture, nullptr, (void**)&textureBuffer, &pitch);
    for (uint32_t i = 0; i < stringLength; i++)
        glyph_to_rgba(char_to_glyph(text.string[i]), text.color, textureBuffer, i, stringLength);
    SDL_UnlockTexture(textTexture);

    SDL_FRect textDstRect {
        scaling * static_cast<float>(text.x),
        scaling * static_cast<float>(text.y),
        scaling * static_cast<float>(textWidth),
        scaling * static_cast<float>(textHeight),
    };

    SDL_RenderTexture(renderer, textTexture, nullptr, &textDstRect);
    SDL_DestroyTexture(textTexture);
}

bool Window::screenshot(const std::string& filename) const {
    auto buffer_rgb888 = create_image_buffer(Specs::Display::WIDTH, Specs::Display::HEIGHT, ImageFormat::RGB888);
    convert_image(ImageFormat::RGB565, framebuffer, ImageFormat::RGB888, buffer_rgb888.data(), Specs::Display::WIDTH,
                  Specs::Display::HEIGHT);
    return save_png_rgb888(filename, buffer_rgb888.data(), Specs::Display::WIDTH, Specs::Display::HEIGHT);
}
Window::Geometry Window::getGeometry() const {
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return {x, y, scaling};
}
