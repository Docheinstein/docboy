#include "window.h"
#include "SDL3/SDL.h"
#include "docboy/shared/specs.h"
#include "screens/screen.h"
#include "utils/asserts.h"

Window::Window(const Geometry& geometry) :
    width(geometry.scaling * Specs::Display::WIDTH),
    height(geometry.scaling * Specs::Display::HEIGHT),
    scaling(geometry.scaling) {

    const int winX = static_cast<int>(geometry.x == WINDOW_POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : geometry.x);
    const int winY = static_cast<int>(geometry.y == WINDOW_POSITION_UNDEFINED ? SDL_WINDOWPOS_UNDEFINED : geometry.y);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "DocBoy");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, winX);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, winY);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, static_cast<float>(width));
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, static_cast<float>(height));
    window = SDL_CreateWindowWithProperties(props);
    if (!window)
        fatal("SDL_CreateWindow error: " + SDL_GetError());

    renderer = SDL_CreateRenderer(window, nullptr, 0);
    if (!renderer)
        fatal("SDL_CreateRenderer error: " + SDL_GetError());
}

void Window::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    screenStack.top->render();

    SDL_RenderPresent(renderer);
}

void Window::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void Window::handleEvent(const SDL_Event& event) const {
    screenStack.top->handleEvent(event);
}

Window::Geometry Window::getGeometry() const {
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return {x, y, scaling};
}

SDL_Renderer* Window::getRenderer() const {
    return renderer;
}

ScreenStack& Window::getScreenStack() {
    return screenStack;
}

void Window::setScaling(uint32_t scaling_) {
    scaling = scaling_;
    SDL_SetWindowSize(window, static_cast<int>(scaling * Specs::Display::WIDTH),
                      static_cast<int>(scaling * Specs::Display::HEIGHT));
}

uint32_t Window::getScaling() const {
    return scaling;
}
