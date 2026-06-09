#ifndef WINDOW_H
#define WINDOW_H

#include <cstdint>

#include "screens/screenstack.h"

class SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
union SDL_Event;

class Window {
public:
    struct Position {
        int x;
        int y;
    };

    static constexpr int POSITION_UNDEFINED = -1;

    explicit Window(int width, int height, int x = POSITION_UNDEFINED, int y = POSITION_UNDEFINED,
                    uint32_t scaling = 1);

    void render();
    void clear();

    void handle_event(const SDL_Event& event) const;

    SDL_Renderer* get_renderer() const;

    ScreenStack& get_screen_stack();

    Position get_position() const;

    uint32_t get_width() const {
        return width;
    }

    uint32_t get_height() const {
        return height;
    }

    void set_scaling(uint32_t scaling);
    uint32_t get_scaling() const {
        return scaling;
    }

    uint32_t get_scaled_width() const {
        return scaled_width;
    }

    uint32_t get_scaled_height() const {
        return scaled_height;
    }

private:
    ScreenStack screen_stack;

    SDL_Window* window;
    SDL_Renderer* renderer;

    uint32_t width;
    uint32_t height;
    uint32_t scaling;

    uint32_t scaled_width;
    uint32_t scaled_height;
};

#endif // WINDOW_H
