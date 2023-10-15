#ifndef WINDOW_H
#define WINDOW_H

class SDL_Window;
class SDL_Renderer;
class SDL_Texture;

#include "docboy/shared/specs.h"
#include <chrono>
#include <cstdint>
#include <list>
#include <optional>
#include <string>

class Window {
public:
    static constexpr int WINDOW_POSITION_UNDEFINED = -1;
    static constexpr uint32_t WINDOW_WIDTH = Specs::Display::WIDTH;
    static constexpr uint32_t WINDOW_HEIGHT = Specs::Display::HEIGHT;
    static constexpr uint64_t TEXT_LETTER_WIDTH = 8;
    static constexpr uint64_t TEXT_LETTER_HEIGHT = 8;
    static constexpr std::optional<uint64_t> TEXT_DURATION_PERSISTENT = std::nullopt;

    explicit Window(const uint16_t* framebuffer, int x = WINDOW_POSITION_UNDEFINED, int y = WINDOW_POSITION_UNDEFINED,
                    float scaling = 1.0);
    ~Window();

    void render();
    void clear();

    uint64_t addText(const std::string& string, int x, int y, uint32_t color = 0x000000FF,
                     std::optional<uint64_t> millisAlive = TEXT_DURATION_PERSISTENT,
                     std::optional<uint64_t> guid = std::nullopt);
    void removeText(uint64_t guid);

    bool screenshot(const std::string& filename) const;

private:
    struct Text {
        uint64_t guid;
        std::string string;
        int x;
        int y;
        uint32_t color;
        std::optional<std::chrono::high_resolution_clock::time_point> deadline;
    };

    void drawFramebuffer();
    void drawTexts();
    void drawText(const Text& text);

    const uint16_t* framebuffer;

    std::list<Text> texts;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    int width;
    int height;
    float scaling;
};

#endif // WINDOW_H