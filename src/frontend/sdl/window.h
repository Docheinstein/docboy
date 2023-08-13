#ifndef WINDOW_H
#define WINDOW_H

#include "core/definitions.h"
#include "core/lcd/framebufferlcd.h"
#include <chrono>
#include <list>

class SDL_Window;
class SDL_Renderer;
class SDL_Texture;

class Window {
public:
    static constexpr int WINDOW_POSITION_UNDEFINED = -1;
    static constexpr uint32_t WINDOW_WIDTH = Specs::Display::WIDTH;
    static constexpr uint32_t WINDOW_HEIGHT = Specs::Display::HEIGHT;
    static constexpr uint64_t TEXT_LETTER_WIDTH = 8;
    static constexpr uint64_t TEXT_LETTER_HEIGHT = 8;
    static constexpr uint64_t TEXT_DURATION_PERSISTENT = UINT64_MAX;

    explicit Window(uint32_t* framebuffer, ILCDIO& lcd, int x = WINDOW_POSITION_UNDEFINED,
                    int y = WINDOW_POSITION_UNDEFINED, float scaling = 1.0);
    ~Window();

    uint64_t addText(const std::string& string, int x, int y, uint32_t color = 0x000000FF, uint64_t numFramesAlive = 1,
                     std::optional<uint64_t> guid = std::nullopt);
    void removeText(uint64_t guid);

    void render();

    [[nodiscard]] std::pair<int, int> getPosition() const;

private:
    struct Text {
        uint64_t guid;
        std::string string;
        int x;
        int y;
        uint32_t color;
        uint64_t remainingFrames;
    };

    uint32_t* framebuffer;
    ILCDIO& lcd;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    int width, height;
    float scaling;

    std::list<Text> texts;

    std::chrono::high_resolution_clock::time_point lastRender;

    void drawFramebuffer();
    void drawTexts();
    void drawText(const Text& text);
};

#endif // WINDOW_H