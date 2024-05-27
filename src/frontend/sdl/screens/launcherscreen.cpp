#include "launcherscreen.h"
#include "SDL3/SDL.h"
#include "controllers/corecontroller.h"
#include "controllers/uicontroller.h"
#include "gamescreen.h"
#include "mainscreen.h"
#include "utils/rompicker.h"

LauncherScreen::LauncherScreen(Context context) :
    Screen(context),
    backgroundTexture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                        Specs::Display::WIDTH, Specs::Display::HEIGHT)),
    foregroundTexture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                        Specs::Display::WIDTH, Specs::Display::HEIGHT)) {
    SDL_SetTextureScaleMode(backgroundTexture, SDL_SCALEMODE_NEAREST);

    SDL_SetTextureScaleMode(foregroundTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(foregroundTexture, SDL_BLENDMODE_ADD);
}

void LauncherScreen::render() {
    SDL_RenderTexture(renderer, backgroundTexture, nullptr, nullptr);
    SDL_RenderTexture(renderer, foregroundTexture, nullptr, nullptr);
}

void LauncherScreen::redraw() {
    clearTexture(backgroundTexture, Specs::Display::WIDTH * Specs::Display::HEIGHT,
                 ui.getCurrentPalette().rgba8888.palette[2] & (0xFFFFFF00 | context.ui.backgroundAlpha));

    uint32_t textColor = ui.getCurrentPalette().rgba8888.palette[0];

    drawText(foregroundTexture, "Press ESC for menu", 9, 18, textColor, Specs::Display::WIDTH);

#ifdef NFD
    drawText(foregroundTexture, "Press ENTER or", 27, 90, textColor, Specs::Display::WIDTH);
    drawText(foregroundTexture, "drop a file to", 27, 106, textColor, Specs::Display::WIDTH);
    drawText(foregroundTexture, "load a GB ROM ", 27, 122, textColor, Specs::Display::WIDTH);
#else
    drawText(foregroundTexture, "Drop a GB ROM", 27, 118, textColor, Specs::Display::WIDTH);
#endif
}

void LauncherScreen::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            nav.push(std::make_unique<MainScreen>(context));
            break;
#ifdef NFD
        case SDLK_RETURN:
            if (const auto rom = openRomPicker()) {
                core.loadRom(*rom);
                nav.push(std::make_unique<GameScreen>(context));
            }
            break;
#endif
        }
        break;
    case SDL_EVENT_DROP_FILE: {
        core.loadRom(event.drop.data);
        nav.push(std::make_unique<GameScreen>(context));
        break;
    }
    }
}