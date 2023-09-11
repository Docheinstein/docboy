#include "window.h"
#include "glyphs.h"
#include "helpers.h"
#include "utils/binutils.h"
#include "utils/exceptionutils.h"
#include <SDL.h>
#include <stdexcept>

Window::~Window() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

Window::Window(uint16_t* framebuffer, ILCDIO& lcd, int x, int y, float scaling) :
    framebuffer(framebuffer),
    lcd(lcd),
    window(),
    renderer(),
    texture(),
    width(static_cast<int>(scaling * WINDOW_WIDTH)),
    height(static_cast<int>(scaling * WINDOW_HEIGHT)),
    scaling(scaling) {

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        THROW(std::runtime_error(std::string("SDL_Init error: ") + SDL_GetError()));

    window =
        SDL_CreateWindow("DocBoy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (!window)
        THROW(std::runtime_error(std::string("SDL_CreateWindow error: ") + SDL_GetError()));

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        THROW(std::runtime_error(std::string("SDL_CreateRenderer error: ") + SDL_GetError()));

    // TODO: figure out byte order
    texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!texture)
        THROW(std::runtime_error(std::string("SDL_CreateTexture error: ") + SDL_GetError()));

    // TODO: don't know but setting these in SDL_CreateWindow does not work
    const auto winX = static_cast<int>(x == WINDOW_POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : x);
    const auto winY = static_cast<int>(y == WINDOW_POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : y);
    SDL_SetWindowPosition(window, winX, winY);
}

uint64_t Window::addText(const std::string& text, int x, int y, uint32_t color, uint64_t numFramesAlive,
                         std::optional<uint64_t> optionalGuid) {
    uint64_t guid;
    if (optionalGuid) {
        guid = *optionalGuid;
        removeText(guid);
    } else {
        guid = rand(); // NOLINT(cert-msc50-cpp)
    }
    texts.emplace_back(guid, text, x, y, color, numFramesAlive);
    return guid;
}

void Window::removeText(uint64_t guid) {
    std::erase_if(texts, [guid](const Text& t) {
        return t.guid == guid;
    });
}

std::pair<int, int> Window::getPosition() const {
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return {x, y};
}

void Window::render() {
    auto now = std::chrono::high_resolution_clock::now();
    if (now < lastRender + std::chrono::milliseconds(16)) {
        return;
    }
    lastRender = now;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    drawFramebuffer();
    drawTexts();

    SDL_RenderPresent(renderer);
}

void Window::drawFramebuffer() {
    bool lcdOn = get_bit<Bits::LCD::LCDC::LCD_ENABLE>(lcd.readLCDC());
    if (lcdOn) {
        SDL_Rect srcrect {.x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT};
        SDL_Rect dstrect {.x = 0, .y = 0, .w = width, .h = height};
        void* texturePixels;
        int pitch;

        SDL_LockTexture(texture, &srcrect, (void**)&texturePixels, &pitch);
        memcpy(texturePixels, framebuffer, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(uint16_t));
        SDL_UnlockTexture(texture);

        SDL_RenderCopy(renderer, texture, nullptr, &dstrect);
    }
}

void Window::drawTexts() {
    auto it = texts.begin();
    while (it != texts.end()) {
        Text& text = *it;
        drawText(text);
        text.remainingFrames--;
        if (text.remainingFrames == 0)
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
        .x = static_cast<int>(scaling * static_cast<float>(text.x)),
        .y = static_cast<int>(scaling * static_cast<float>(text.y)),
        .w = static_cast<int>(scaling * static_cast<float>(textWidth)),
        .h = static_cast<int>(scaling * static_cast<float>(textHeight)),
    };

    SDL_RenderCopy(renderer, textTexture, nullptr, &textDstRect);
    SDL_DestroyTexture(textTexture);
}