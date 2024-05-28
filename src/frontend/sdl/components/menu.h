#ifndef MENU_H
#define MENU_H

#include "SDL3/SDL_keycode.h"
#include "text.h"
#include "utils/textures.h"
#include "utils/vector.hpp"
#include <functional>
#include <string>

struct SDL_Texture;

class Menu {
public:
    struct MenuItem {
        std::string text {};
        std::function<void()> onEnter {};
        std::function<void()> onPrev {};
        std::function<void()> onNext {};
    };

    explicit Menu(SDL_Texture* texture, const std::array<uint32_t, 4>& palette);

    MenuItem& addItem(MenuItem&& item);

    void setNavigationEnabled(bool enabled);

    void redraw();

    void handleInput(SDL_Keycode key);

private:
    SDL_Texture* texture {};
    int height {};
    int width {};

    const std::array<uint32_t, 4>& palette;

    Vector<MenuItem, 16> items;
    uint8_t cursor {};

    bool navigation {true};
};

#endif // MENU_H
