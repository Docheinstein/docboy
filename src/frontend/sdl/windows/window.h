#ifndef WINDOW_H
#define WINDOW_H

#include <cstdint>

#include "controllers/navcontroller.h"
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

    uint32_t get_width() const;
    uint32_t get_height() const;

    void set_scaling(uint32_t scaling);
    uint32_t get_scaling() const;

private:
    ScreenStack screen_stack;

    SDL_Window* window;
    SDL_Renderer* renderer;

    uint32_t width;
    uint32_t height;
    uint32_t scaling;
};

#endif // WINDOW_H
