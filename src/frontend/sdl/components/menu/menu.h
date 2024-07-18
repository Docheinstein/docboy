#ifndef MENU_H
#define MENU_H

#include <array>
#include <functional>
#include <string>
#include <utility>

#include "components/menu/item.h"
#include "primitives/text.h"

#include "utils/textures.h"
#include "utils/vector.h"

#include "SDL3/SDL_keycode.h"

struct SDL_Texture;

class Menu {
public:
    explicit Menu(SDL_Texture* texture, const std::array<uint32_t, 4>& palette);

    MenuItem& add_item(MenuItem&& item);

    void redraw();

    void invalidate_layout();

    void handle_input(SDL_Keycode key);

private:
    SDL_Texture* texture {};
    int height {};
    int width {};

    uint8_t max_viewport_items {};

    const std::array<uint32_t, 4>& palette;

    Vector<MenuItem, 16> items;
    Vector<MenuItem*, 16> visible_items;
    uint8_t cursor {};

    uint8_t viewport_cursor {};

    bool navigation {true};
};

#endif // MENU_H
