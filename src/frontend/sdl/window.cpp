#include "window.h"
#include <SDL.h>
#include <stdexcept>

Window::Window() {

}

Window::~Window() {
    SDL_FreeSurface(surface);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window::show() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(std::string("SDL_Init error: ") + SDL_GetError());
    }

    window = SDL_CreateWindow("Gible", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 480, 480, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        throw std::runtime_error(std::string("SDL_CreateWindow error: ") + SDL_GetError());
    }

    surface = SDL_GetWindowSurface(window);
//    pixels = (unsigned int *) surface->pixels;
//    format = surface->format;
}


