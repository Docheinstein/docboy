#include "window.h"
#include "SDL3/SDL.h"
#include "docboy/common/specs.h"
#include "screens/screen.h"
#include "utils/asserts.h"

Window::Window(int width, int height, int x, int y, uint32_t scaling) :
    width {static_cast<uint32_t>(width)},
    height {static_cast<uint32_t>(height)},
    scaling {scaling} {

    const int win_x = x == POSITION_UNDEFINED ? static_cast<int>(SDL_WINDOWPOS_UNDEFINED) : x;
    const int win_y = y == POSITION_UNDEFINED ? static_cast<int>(SDL_WINDOWPOS_UNDEFINED) : y;

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "DocBoy");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, win_x);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, win_y);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, static_cast<float>(width * scaling));
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, static_cast<float>(height * scaling));

    window = SDL_CreateWindowWithProperties(props);
    if (!window) {
        FATAL("SDL_CreateWindow error: " + SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        FATAL("SDL_CreateRenderer error: " + SDL_GetError());
    }
}

void Window::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    screen_stack.top->render();

    SDL_RenderPresent(renderer);
}

void Window::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void Window::handle_event(const SDL_Event& event) const {
    screen_stack.top->handle_event(event);
}

SDL_Renderer* Window::get_renderer() const {
    return renderer;
}

ScreenStack& Window::get_screen_stack() {
    return screen_stack;
}

Window::Position Window::get_position() const {
    Position pos {};
    SDL_GetWindowPosition(window, &pos.x, &pos.y);
    return pos;
}

uint32_t Window::get_width() const {
    return width;
}

uint32_t Window::get_height() const {
    return height;
}

void Window::set_scaling(uint32_t scaling_) {
    scaling = scaling_;
    SDL_SetWindowSize(window, static_cast<int>(width * scaling), static_cast<int>(height * scaling));
}

uint32_t Window::get_scaling() const {
    return scaling;
}
