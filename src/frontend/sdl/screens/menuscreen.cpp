#include "screens/menuscreen.h"

#include "controllers/navcontroller.h"
#include "controllers/uicontroller.h"

#include "docboy/common/specs.h"

#include "SDL3/SDL.h"

MenuScreen::MenuScreen(Context context) :
    Screen {context},
    menu_background_texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                               static_cast<int>(ui.get_width()), static_cast<int>(ui.get_height()))},
    menu_foreground_texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                               static_cast<int>(ui.get_width()), static_cast<int>(ui.get_height()))},
    menu {menu_foreground_texture, ui.get_current_appearance().menu} {

    SDL_SetTextureScaleMode(menu_background_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(menu_background_texture, SDL_BLENDMODE_BLEND);

    SDL_SetTextureScaleMode(menu_foreground_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(menu_foreground_texture, SDL_BLENDMODE_ADD);
}

void MenuScreen::redraw() {
    uint32_t* texture_buffer = lock_texture(menu_background_texture);
    clear_texture(texture_buffer, ui.get_width() * ui.get_height(),
                  ui.get_current_appearance().menu[2] & (0xFFFFFF00 | context.ui.background_alpha));
    unlock_texture(menu_background_texture);
    menu.redraw();
}

void MenuScreen::render() {
    SDL_RenderTexture(renderer, menu_background_texture, nullptr, nullptr);
    SDL_RenderTexture(renderer, menu_foreground_texture, nullptr, nullptr);
}

void MenuScreen::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        const auto key {event.key.key};
        if (key == SDLK_ESCAPE || key == SDLK_BACKSPACE) {
            nav.pop();
        } else {
            menu.handle_input(key);
        }
    }
}
