#include "menu.h"

#include "components/glyphs.h"

#include "SDL3/SDL.h"

namespace {
constexpr Glyph NAVIGATION_CURSOR_GLYPH = 0x80C0E0F0E0C08000 /* arrow */;
constexpr uint8_t V_MARGIN = 12;
constexpr uint8_t H_MARGIN = 12;
constexpr uint8_t TEXT_WIDTH = 8;
constexpr uint8_t TEXT_HEIGHT = 8;
constexpr uint8_t TEXT_V_SPACING = 6;
constexpr uint8_t TEXT_ROW_HEIGHT = TEXT_HEIGHT + TEXT_V_SPACING;
} // namespace

Menu::Menu(SDL_Texture* texture, const std::array<uint32_t, 4>& palette) :
    texture {texture},
    palette {palette} {
    // Deduce the menu size from the texture
    float w, h;
    SDL_GetTextureSize(texture, &w, &h);

    width = static_cast<int>(w);
    height = static_cast<int>(h);

    max_viewport_items = static_cast<uint8_t>((height - (2 * V_MARGIN)) / TEXT_ROW_HEIGHT);
}

MenuItem& Menu::add_item(MenuItem&& item) {
    ASSERT(items.is_not_full());
    items.emplace_back(std::move(item));
    MenuItem& it = items[items.size() - 1];
    if (it.visible) {
        visible_items.push_back(&it);
    }
    return it;
}

void Menu::redraw() {
    clear_texture(texture, width * height);

    const uint8_t max_items = std::min(visible_items.size(), max_viewport_items);

    // Draw items
    for (uint32_t i = 0; i < max_items; i++) {
        const MenuItem& item = *visible_items[viewport_cursor + i];
        uint8_t text_size = TEXT_WIDTH * item.text.size();
        uint8_t text_x = 0;

        // Eventually justify text
        if (item.justification == MenuItem::Justification::Left) {
            text_x = H_MARGIN;
        } else if (item.justification == MenuItem::Justification::Center) {
            text_x = (width - text_size) / 2;
        } else if (item.justification == MenuItem::Justification::Right) {
            text_x = (width - H_MARGIN - text_size);
        }

        draw_text(texture, item.text, text_x, V_MARGIN + TEXT_ROW_HEIGHT * i, palette[0], width);
    }

    // Draw cursor
    ASSERT(cursor >= viewport_cursor && cursor < viewport_cursor + max_viewport_items);
    uint8_t cursor_viewport_space = cursor - viewport_cursor;
    draw_glyph(texture, NAVIGATION_CURSOR_GLYPH, 4, V_MARGIN + TEXT_ROW_HEIGHT * cursor_viewport_space, palette[0],
               width);
}

void Menu::handle_input(SDL_Keycode key) {
    const auto goToNextSelectableItem = [this](int8_t increment) {
        ASSERT(visible_items[cursor]->selectable);

        uint8_t next_cursor = cursor;
        do {
            if (next_cursor + increment < 0 || next_cursor + increment >= visible_items.size()) {
                break;
            }
            next_cursor += increment;
        } while (!visible_items[next_cursor]->selectable);

        if (next_cursor == cursor) {
            return;
        }

        cursor = next_cursor;

        if (visible_items.size() > max_viewport_items) {
            if (increment > 0) {
                if (cursor >= (viewport_cursor + max_viewport_items)) {
                    viewport_cursor = std::max(0, cursor - max_viewport_items + 1);
                    cursor = viewport_cursor + max_viewport_items - 1;
                }
            } else {
                if (cursor < viewport_cursor) {
                    viewport_cursor = cursor;
                }
            }
        }
    };

    switch (key) {
    case SDLK_UP:
        goToNextSelectableItem(-1);
        redraw();
        break;
    case SDLK_DOWN:
        goToNextSelectableItem(1);
        redraw();
        break;
    case SDLK_LEFT:
        if (visible_items[cursor]->on_prev) {
            visible_items[cursor]->on_prev();
        }
        break;
    case SDLK_RIGHT:
        if (visible_items[cursor]->on_next) {
            visible_items[cursor]->on_next();
        }
        break;
    case SDLK_RETURN:
        if (visible_items[cursor]->on_enter) {
            visible_items[cursor]->on_enter();
        }
        break;
    default:
        break;
    }
}

void Menu::invalidate_layout() {
    visible_items.clear();
    for (auto& item : items) {
        if (item.visible) {
            visible_items.push_back(&item);
        }
    }
}
