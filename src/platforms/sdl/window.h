#ifndef WINDOW_H
#define WINDOW_H

class SDL_Window;
class SDL_Surface;

class Window {
public:
    Window();
    ~Window();

    void show();

private:
    SDL_Window *window;
    SDL_Surface *surface;
//    SDL_PixelFormat *format;
};

#endif // WINDOW_H