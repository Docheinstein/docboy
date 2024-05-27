#ifndef WINDOW_H
#define WINDOW_H

#include "controllers/navcontroller.h"
#include "screens/screenstack.h"
#include <cstdint>

class SDL_Window;
class SDL_Renderer;
struct SDL_Texture;
union SDL_Event;

class Window {
public:
    struct Geometry {
        int x {};
        int y {};
        uint32_t scaling {};
    };

    static constexpr int WINDOW_POSITION_UNDEFINED = -1;
    static constexpr Geometry DEFAULT_GEOMETRY = {WINDOW_POSITION_UNDEFINED, WINDOW_POSITION_UNDEFINED, 1};

    explicit Window(const Geometry& geometry = DEFAULT_GEOMETRY);

    void render();
    void clear();

    void handleEvent(const SDL_Event& event) const;

    Geometry getGeometry() const;

    SDL_Renderer* getRenderer() const;

    ScreenStack& getScreenStack();

    void setScaling(uint32_t scaling);
    uint32_t getScaling() const;

private:
    ScreenStack screenStack;

    SDL_Window* window;
    SDL_Renderer* renderer;

    uint32_t width;
    uint32_t height;
    uint32_t scaling;
};

#endif // WINDOW_H
