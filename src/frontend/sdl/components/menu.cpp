#include "components/menu.h"

#include "components/glyphs.h"

#include "SDL3/SDL.h"

namespace {
constexpr Glyph NAVIGATION_CURSOR_GLYPH = 0x80C0E0F0E0C08000 /* arrow */;
}

Menu::Menu(SDL_Texture* texture, const std::array<uint32_t, 4>& palette) :
    texture {texture},
    palette {palette} {
    // Deduce the menu size from the texture
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
}

void Menu::set_navigation_enabled(bool enabled) {
    navigation = enabled;
}

void Menu::redraw() {
    clear_texture(texture, width * height);

    // Draw items
    for (uint32_t i = 0; i < items.size(); i++) {
        draw_text(texture, items[i].text, 12, 12 + 14 * i, palette[0], width);
    }

    // Draw cursor
    if (navigation) {
        draw_glyph(texture, NAVIGATION_CURSOR_GLYPH, 4, 12 + 14 * cursor, palette[0], width);
    }
}

void Menu::handle_input(SDL_Keycode key) {
    if (!navigation) {
        return;
    }

    switch (key) {
    case SDLK_UP:
        cursor = (cursor + items.size() - 1) % items.size();
        redraw();
        break;
    case SDLK_DOWN:
        cursor = (cursor + 1) % items.size();
        redraw();
        break;
    case SDLK_LEFT:
        if (items[cursor].on_prev_fn) {
            items[cursor].on_prev_fn();
        }
        break;
    case SDLK_RIGHT:
        if (items[cursor].on_next_fn) {
            items[cursor].on_next_fn();
        }
        break;
    case SDLK_RETURN:
        if (items[cursor].on_enter_fn) {
            items[cursor].on_enter_fn();
        }
        break;
    default:
        break;
    }
}
