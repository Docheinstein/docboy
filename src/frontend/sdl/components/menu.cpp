#include "menu.h"
#include "SDL3/SDL.h"
#include "components/glyphs.h"

namespace {
constexpr Glyph NAVIGATION_CURSOR_GLYPH = 0x80c0e0f0e0c08000 /* arrow */;
}

Menu::Menu(SDL_Texture* texture, const std::array<uint32_t, 4>& palette) :
    texture(texture),
    palette(palette) {
    // Deduce the menu size from the texture
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
}

Menu::MenuItem& Menu::addItem(Menu::MenuItem&& item) {
    items.emplaceBack(std::move(item));
    return items[items.size() - 1];
}

void Menu::setNavigationEnabled(bool enabled) {
    navigation = enabled;
}

void Menu::redraw() {
    clearTexture(texture, width * height);

    // Draw items
    for (uint32_t i = 0; i < items.size(); i++)
        drawText(texture, items[i].text, 12, 12 + 14 * i, palette[0], width);

    // Draw cursor
    if (navigation)
        drawGlyph(texture, NAVIGATION_CURSOR_GLYPH, 4, 12 + 14 * cursor, palette[0], width);
}

void Menu::handleInput(SDL_Keycode key) {
    if (!navigation)
        return;

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
        if (items[cursor].onPrev)
            items[cursor].onPrev();
        break;
    case SDLK_RIGHT:
        if (items[cursor].onNext)
            items[cursor].onNext();
        break;
    case SDLK_RETURN:
        if (items[cursor].onEnter)
            items[cursor].onEnter();
        break;
    default:
        break;
    }
}
