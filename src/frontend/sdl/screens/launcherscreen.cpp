#include "screens/launcherscreen.h"

#include "controllers/corecontroller.h"
#include "controllers/uicontroller.h"
#include "screens/gamescreen.h"
#include "screens/mainscreen.h"

#include "utils/rompicker.h"

#include "SDL3/SDL.h"

LauncherScreen::LauncherScreen(Context context) :
    Screen {context},
    background_texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                          Specs::Display::WIDTH, Specs::Display::HEIGHT)},
    foreground_texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                          Specs::Display::WIDTH, Specs::Display::HEIGHT)} {
    SDL_SetTextureScaleMode(background_texture, SDL_SCALEMODE_NEAREST);

    SDL_SetTextureScaleMode(foreground_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(foreground_texture, SDL_BLENDMODE_ADD);
}

void LauncherScreen::render() {
    SDL_RenderTexture(renderer, background_texture, nullptr, nullptr);
    SDL_RenderTexture(renderer, foreground_texture, nullptr, nullptr);
}

void LauncherScreen::redraw() {
    clear_texture(background_texture, Specs::Display::WIDTH * Specs::Display::HEIGHT,
                  ui.get_current_palette().rgba8888.palette[2] & (0xFFFFFF00 | context.ui.background_alpha));

    uint32_t text_color = ui.get_current_palette().rgba8888.palette[0];

    draw_text(foreground_texture, "Press ESC for menu", 9, 18, text_color, Specs::Display::WIDTH);

#ifdef NFD
    draw_text(foreground_texture, "Press ENTER or", 27, 90, text_color, Specs::Display::WIDTH);
    draw_text(foreground_texture, "drop a file to", 27, 106, text_color, Specs::Display::WIDTH);
    draw_text(foreground_texture, "load a GB ROM ", 27, 122, text_color, Specs::Display::WIDTH);
#else
    draw_text(foreground_texture, "Drop a GB ROM", 27, 118, text_color, Specs::Display::WIDTH);
#endif
}

void LauncherScreen::handle_event(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            nav.push(std::make_unique<MainScreen>(context));
            break;
#ifdef NFD
        case SDLK_RETURN:
            if (const auto rom = open_rom_picker()) {
                core.load_rom(*rom);
                nav.push(std::make_unique<GameScreen>(context));
            }
            break;
#endif
        }
        break;
    case SDL_EVENT_DROP_FILE: {
        core.load_rom(event.drop.data);
        nav.push(std::make_unique<GameScreen>(context));
        break;
    }
    }
}