#include "menuscreen.h"
#include "SDL3/SDL.h"
#include "controllers/navcontroller.h"
#include "controllers/uicontroller.h"
#include "docboy/shared/specs.h"

MenuScreen::MenuScreen(Context context) :
    Screen(context),
    menuBackgroundTexture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                            Specs::Display::WIDTH, Specs::Display::HEIGHT)),
    menuForegroundTexture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                            Specs::Display::WIDTH, Specs::Display::HEIGHT)),
    menu(menuForegroundTexture, ui.getCurrentPalette().rgba8888.palette) {

    SDL_SetTextureScaleMode(menuBackgroundTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(menuBackgroundTexture, SDL_BLENDMODE_BLEND);

    SDL_SetTextureScaleMode(menuForegroundTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(menuForegroundTexture, SDL_BLENDMODE_ADD);
}

void MenuScreen::redraw() {
    clearTexture(menuBackgroundTexture, Specs::Display::WIDTH * Specs::Display::HEIGHT,
                 ui.getCurrentPalette().rgba8888.palette[2] & (0xFFFFFF00 | context.ui.backgroundAlpha));
    menu.redraw();
}

void MenuScreen::render() {
    SDL_RenderTexture(renderer, menuBackgroundTexture, nullptr, nullptr);
    SDL_RenderTexture(renderer, menuForegroundTexture, nullptr, nullptr);
}

void MenuScreen::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        const auto key {event.key.keysym.sym};
        if (key == SDLK_ESCAPE || key == SDLK_BACKSPACE)
            nav.pop();
        else
            menu.handleInput(key);
    }
}
