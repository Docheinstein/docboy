#include "screens/gamescreen.h"

#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/uicontroller.h"
#include "screens/mainscreen.h"

#include "extra/img/imgmanip.h"
#include "extra/png/iopng.h"

#include "utils/io.h"
#include "utils/rompicker.h"

#include "SDL3/SDL.h"
#include "gamemainscreen.h"

#ifdef ENABLE_DEBUGGER
#include "controllers/debuggercontroller.h"
#endif

GameScreen::GameScreen(Context context) :
    Screen {context},
    game_texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                    Specs::Display::WIDTH, Specs::Display::HEIGHT)},
    game_overlay_texture {SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                            Specs::Display::WIDTH, Specs::Display::HEIGHT)},
    game_framebuffer {core.get_framebuffer()} {

    SDL_SetTextureScaleMode(game_texture, SDL_SCALEMODE_NEAREST);

    SDL_SetTextureScaleMode(game_overlay_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(game_overlay_texture, SDL_BLENDMODE_BLEND);

    ASSERT(core.is_rom_loaded());

    // Exit pause state (start emulation)
    core.set_paused(false);

    // Pause the emulation if menu is visible
    menu.screen_stack.set_on_screen_changed_callback([this]() {
        core.set_paused(is_in_menu());
    });
}

void GameScreen::redraw() {
    if (is_in_menu()) {
        menu.screen_stack.top->redraw();
        return;
    }

    redraw_overlay();
}

void GameScreen::render() {
    if (is_in_menu()) {
        // Draw game framebuffer even in menu: it will be shown underneath the semi-transparent menu
        SDL_RenderTexture(renderer, game_texture, nullptr, nullptr);
        menu.screen_stack.top->render();
        return;
    }

    // Copy framebuffer to rendered texture
    copy_to_texture(game_texture, game_framebuffer, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t));

    // Update FPS
    if (fps.visible) {
        fps.count++;
        const auto now = std::chrono::high_resolution_clock::now();
        if (now - fps.last_sample > std::chrono::nanoseconds(1000000000)) {
            fps.display_count = fps.count;
            fps.last_sample = now;
            fps.count = 0;
            redraw_overlay();
        }
    }

    // Eventually remove expired popup
    if (popup.visible && std::chrono::high_resolution_clock::now() > popup.deadline) {
        popup.visible = false;
        redraw_overlay();
    }

    SDL_RenderTexture(renderer, game_texture, nullptr, nullptr);
    SDL_RenderTexture(renderer, game_overlay_texture, nullptr, nullptr);
}

void GameScreen::handle_event(const SDL_Event& event) {
    if (is_in_menu()) {
        menu.screen_stack.top->handle_event(event);
        return;
    }

    ASSERT(!core.is_paused());

    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        switch (event.key.key) {
        case SDLK_F1:
            if (core.write_state()) {
                draw_popup("State saved");
            }
            break;
        case SDLK_F2:
            if (core.load_state()) {
                draw_popup("State loaded");
            }
            break;
        case SDLK_F11: {
            bool ok {};
            write_file((temp_directory_path() / core.get_rom().with_extension("dat").filename()).string(),
                       game_framebuffer, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t), &ok);
            if (ok) {
                draw_popup("Framebuffer saved");
            }
            break;
        }
        case SDLK_F12: {
            auto buffer_rgb888 =
                create_image_buffer(Specs::Display::WIDTH, Specs::Display::HEIGHT, ImageFormat::RGB888);
            convert_image(ImageFormat::RGB565, game_framebuffer, ImageFormat::RGB888, buffer_rgb888.data(),
                          Specs::Display::WIDTH, Specs::Display::HEIGHT);
            if (save_png_rgb888((temp_directory_path() / core.get_rom().with_extension("png").filename()).string(),
                                buffer_rgb888.data(), Specs::Display::WIDTH, Specs::Display::HEIGHT)) {
                draw_popup("Screenshot saved");
            }
            break;
        }
        case SDLK_F:
            fps.visible = !fps.visible;
            fps.last_sample = std::chrono::high_resolution_clock::now();
            fps.display_count = 0;
            fps.count = 0;
            redraw_overlay();
            break;
#ifdef ENABLE_AUDIO
        case SDLK_M:
            main.set_audio_enabled(!main.is_audio_enabled());
            draw_popup(std::string {"Audio "} + (main.is_audio_enabled() ? "enabled" : "disabled"));
            break;
#endif
        case SDLK_Q:
            main.set_speed(main.get_speed() - 1);
            redraw_overlay();
            break;
        case SDLK_W:
            main.set_speed(main.get_speed() + 1);
            redraw_overlay();
            break;
#ifdef ENABLE_DEBUGGER
        case SDLK_D:
            if (debugger.is_debugger_attached()) {
                if (debugger.detach_debugger()) {
                    draw_popup("Debugger detached");
                }
            } else {
                if (debugger.attach_debugger(true)) {
                    draw_popup("Debugger attached");
                }
            }
            break;
#endif
        case SDLK_ESCAPE: {
            // Open menu with a semi-transparent background
#ifdef ENABLE_DEBUGGER
            Screen::Controllers controllers {core, ui, menu.nav_controller, main, debugger};
#else
            Screen::Controllers controllers {core, ui, menu.nav_controller, main};
#endif
            menu.screen_stack.push(std::make_unique<GameMainScreen>(Context {controllers, {0xE0}}));
            redraw();
        }
        default:
            core.send_key(event.key.key, Joypad::KeyState::Pressed);
            break;
        }
        break;
    }
    case SDL_EVENT_KEY_UP: {
        core.send_key(event.key.key, Joypad::KeyState::Released);
        break;
    }
    case SDL_EVENT_DROP_FILE: {
        core.load_rom(event.drop.data);
        break;
    }
    }
}

void GameScreen::draw_popup(const std::string& str) {
    popup.visible = true;
    popup.deadline = std::chrono::high_resolution_clock::now() + std::chrono::seconds(2);
    popup.text = str;
    redraw_overlay();
}

void GameScreen::redraw_overlay() {
    uint32_t text_color = ui.get_current_palette().rgba8888.accent;

    clear_texture(game_overlay_texture, Specs::Display::WIDTH * Specs::Display::HEIGHT);

    if (popup.visible) {
        draw_text(game_overlay_texture, popup.text, 4, Specs::Display::HEIGHT - 12, text_color, Specs::Display::WIDTH);
    }

    if (fps.visible) {
        const std::string fps_str = std::to_string(fps.display_count);
        draw_text(game_overlay_texture, fps_str, Specs::Display::WIDTH - 4 - fps_str.size() * 8, 4, text_color,
                  Specs::Display::WIDTH);
    }

    if (const int32_t speed {main.get_speed()}; speed != 0) {
        draw_text(game_overlay_texture, (speed > 0 ? "x" : "/") + std::to_string((uint32_t)(pow2(abs(speed)))), 4, 4,
                  text_color, Specs::Display::WIDTH);
    }
}

bool GameScreen::is_in_menu() const {
    return menu.screen_stack.top;
}