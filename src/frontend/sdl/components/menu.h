#ifndef MENU_H
#define MENU_H

#include <array>
#include <functional>
#include <string>

#include "components/text.h"

#include "utils/textures.h"
#include "utils/vector.h"

#include "SDL3/SDL_keycode.h"

struct SDL_Texture;

class Menu {
public:
    struct MenuItem {
        std::string text {};
        std::function<void()> on_enter {};
        std::function<void()> on_prev {};
        std::function<void()> on_next {};
    };

    explicit Menu(SDL_Texture* texture, const std::array<uint32_t, 4>& palette);

    MenuItem& add_item(MenuItem&& item);

    void set_navigation_enabled(bool enabled);

    void redraw();

    void handle_input(SDL_Keycode key);

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
