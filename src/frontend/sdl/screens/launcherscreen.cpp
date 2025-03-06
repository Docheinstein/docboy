#include "screens/launcherscreen.h"

#include "controllers/corecontroller.h"
#include "controllers/runcontroller.h"
#include "controllers/uicontroller.h"
#include "primitives/glyph.h"
#include "screens/gamescreen.h"
#include "screens/mainscreen.h"

#include "utils/rompicker.h"

#include "SDL3/SDL.h"

LauncherScreen::LauncherScreen(Context context) :
    Screen {context},
    background_texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                          static_cast<int>(ui.get_width()), static_cast<int>(ui.get_height()))},
    foreground_texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                          static_cast<int>(ui.get_width()), static_cast<int>(ui.get_height()))} {
    SDL_SetTextureScaleMode(background_texture, SDL_SCALEMODE_NEAREST);

    SDL_SetTextureScaleMode(foreground_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(foreground_texture, SDL_BLENDMODE_ADD);
}

void LauncherScreen::render() {
    SDL_RenderTexture(renderer, background_texture, nullptr, nullptr);
    SDL_RenderTexture(renderer, foreground_texture, nullptr, nullptr);
}

void LauncherScreen::redraw() {
    uint32_t* background_texture_buffer = lock_texture(background_texture);
    uint32_t* foreground_texture_buffer = lock_texture(foreground_texture);

    clear_texture(background_texture_buffer, ui.get_width() * ui.get_height(),
                  ui.get_current_palette().rgba8888.palette[2] & (0xFFFFFF00 | context.ui.background_alpha));

    uint32_t text_color = ui.get_current_palette().rgba8888.palette[0];

    const auto draw_text_horizontally_centered = [foreground_texture_buffer, width = ui.get_width(),
                                                  text_color](const char* str, uint32_t y) {
        draw_text(foreground_texture_buffer, width, str, (width - GLYPH_WIDTH * strlen(str)) / 2, y, text_color);
    };

    draw_text_horizontally_centered("Press ESC for menu", 18);

#ifdef NFD
    if (runner.is_two_players_mode()) {
        draw_text_horizontally_centered("Press ENTER or drop a", 90);
        draw_text_horizontally_centered("file to load a GB ROM", 106);
        if (runner.get_core1().is_rom_loaded()) {
            draw_text_horizontally_centered("for GameBoy #2 ", 122);
        } else {
            draw_text_horizontally_centered("for GameBoy #1 ", 122);
        }
    } else {
        draw_text_horizontally_centered("Press ENTER or", 90);
        draw_text_horizontally_centered("drop a file to", 106);
        draw_text_horizontally_centered("load a GB ROM ", 122);
    }
#else
    if (runner.is_two_players_mode()) {
        if (runner.get_core1().is_rom_loaded()) {
            draw_text_horizontally_centered("Drop a GB ROM for GameBoy #2", 118);
        } else {
            draw_text_horizontally_centered("Drop a GB ROM for GameBoy #1", 118);
        }
    } else {
        draw_text_horizontally_centered("Drop a GB ROM", 118);
    }
#endif

    unlock_texture(background_texture);
    unlock_texture(foreground_texture);
}

void LauncherScreen::handle_event(const SDL_Event& event) {
    const auto load_next_rom = [this](const char* rom) {
        bool all_roms_loaded = false;
#ifdef ENABLE_TWO_PLAYERS_MODE
        if (runner.is_two_players_mode()) {
            if (runner.get_core1().is_rom_loaded()) {
                runner.get_core2().load_rom(rom);
                all_roms_loaded = true;
            } else {
                runner.get_core1().load_rom(rom);
            }
        } else
#endif
        {
            runner.get_core1().load_rom(rom);
            all_roms_loaded = true;
        }

        if (all_roms_loaded) {
            nav.push(std::make_unique<GameScreen>(context));
        } else {
            redraw();
        }
    };

    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
        switch (event.key.key) {
        case SDLK_ESCAPE:
            nav.push(std::make_unique<MainScreen>(context));
            break;
#ifdef NFD
        case SDLK_RETURN:
            if (const auto rom = open_rom_picker()) {
                load_next_rom(rom->c_str());
            }
            break;
#endif
        }
        break;
    case SDL_EVENT_DROP_FILE: {
        load_next_rom(event.drop.data);
        break;
    }
    }
}