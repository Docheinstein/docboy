#include "screens/gamescreen.h"

#include "SDL3/SDL.h"

#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/runcontroller.h"
#include "controllers/uicontroller.h"

#include "extra/img/imgmanip.h"
#include "extra/png/iopng.h"

#include "utils/asserts.h"
#include "utils/io.h"

#include "screens/gamemainscreen.h"

#ifdef ENABLE_DEBUGGER
#include "controllers/debuggercontroller.h"
#endif

namespace {
SDL_ScaleMode scaling_filter_to_texture_scale_mode(UiController::ScalingFilter filter) {
    switch (filter) {
    case UiController::ScalingFilter::NearestNeighbor:
        return SDL_SCALEMODE_NEAREST;
    case UiController::ScalingFilter::Linear:
        return SDL_SCALEMODE_LINEAR;
    default:
        ASSERT_NO_ENTRY();
        return SDL_SCALEMODE_NEAREST;
    }
}
} // namespace

GameScreen::GameScreen(Context context) :
    Screen {context} {

    SDL_ScaleMode texture_scale_mode =
        scaling_filter_to_texture_scale_mode(context.controllers.ui.get_scaling_filter());

    game_framebuffer1 = runner.get_core1().get_framebuffer();

    game_texture1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                      Specs::Display::WIDTH, Specs::Display::HEIGHT);
    SDL_SetTextureScaleMode(game_texture1, texture_scale_mode);

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (runner.is_two_players_mode()) {
        game_framebuffer2 = runner.get_core2().get_framebuffer();

        game_texture2 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                          Specs::Display::WIDTH, Specs::Display::HEIGHT);
        SDL_SetTextureScaleMode(game_texture2, texture_scale_mode);
    }
#endif

    game_overlay_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                             static_cast<int>(ui.get_width()), static_cast<int>(ui.get_height()));
    SDL_SetTextureScaleMode(game_overlay_texture, texture_scale_mode);
    SDL_SetTextureBlendMode(game_overlay_texture, SDL_BLENDMODE_BLEND);

    ASSERT(runner.get_core1().is_rom_loaded());

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (runner.is_two_players_mode()) {
        ASSERT(runner.get_core2().is_rom_loaded());
    }
#endif

    // Exit pause state (start emulation)
    runner.set_paused(false);

    // Pause the emulation if menu is visible
    menu.screen_stack.set_on_screen_changed_callback([this] {
        runner.set_paused(is_in_menu());
    });

    // Listen to any change of scaling filter
    context.controllers.ui.set_scaling_filter_changed_callback([this](UiController::ScalingFilter filter) {
        on_scaling_filter_changed(filter);
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
        render_game_texture();
        menu.screen_stack.top->render();
        return;
    }

    // Copy framebuffer to rendered texture
    uint32_t* game_texture_buffer = lock_texture(game_texture1);
    copy_to_texture(game_texture_buffer, game_framebuffer1,
                    Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t));
    unlock_texture(game_texture1);

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (game_framebuffer2) {
        ASSERT(game_texture2);
        uint32_t* game_texture2_buffer = lock_texture(game_texture2);
        copy_to_texture(game_texture2_buffer, game_framebuffer2,
                        Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t));
        unlock_texture(game_texture2);
    }
#endif

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

    render_game_texture();

    SDL_RenderTexture(renderer, game_overlay_texture, nullptr, nullptr);
}

void GameScreen::handle_event(const SDL_Event& event) {
    if (is_in_menu()) {
        menu.screen_stack.top->handle_event(event);
        return;
    }

    ASSERT(!runner.is_paused());

    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        switch (event.key.key) {
        case SDLK_F1:
            if (runner.get_core1().write_state()) {
                draw_popup("State saved");
            }
            break;
        case SDLK_F2:
            if (runner.get_core1().load_state()) {
                draw_popup("State loaded");
            }
            break;
#ifdef ENABLE_TWO_PLAYERS_MODE
        case SDLK_F3:
            if (runner.is_two_players_mode()) {
                if (runner.get_core2().write_state()) {
                    draw_popup("State saved (#2)");
                }
            }
            break;
        case SDLK_F4:
            if (runner.is_two_players_mode()) {
                if (runner.get_core2().load_state()) {
                    draw_popup("State loaded (#2)");
                }
            }
            break;
#endif
        case SDLK_F11: {
            const auto save_framebuffer_dat = [](const PixelRgb565* framebuffer, CoreController& core) {
                bool ok {};
                write_file((temp_directory_path() / core.get_rom().with_extension("dat").filename()).string(),
                           framebuffer, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t), &ok);
                return ok;
            };

            bool ok {};
            ok = save_framebuffer_dat(game_framebuffer1, runner.get_core1());

#ifdef ENABLE_TWO_PLAYERS_MODE
            if (runner.is_two_players_mode()) {
                ok |= save_framebuffer_dat(game_framebuffer2, runner.get_core2());
            }
#endif

            if (ok) {
                draw_popup("Framebuffer saved");
            }
            break;
        }
        case SDLK_F12: {
            const auto save_framebuffer_png = [](const PixelRgb565* framebuffer, CoreController& core) {
                auto buffer_rgb888 =
                    create_image_buffer(Specs::Display::WIDTH, Specs::Display::HEIGHT, ImageFormat::RGB888);
                convert_image(ImageFormat::RGB565, framebuffer, ImageFormat::RGB888, buffer_rgb888.data(),
                              Specs::Display::WIDTH, Specs::Display::HEIGHT);
                return save_png_rgb888(
                    (temp_directory_path() / core.get_rom().with_extension("png").filename()).string(),
                    buffer_rgb888.data(), Specs::Display::WIDTH, Specs::Display::HEIGHT);
            };

            bool ok {};
            ok = save_framebuffer_png(game_framebuffer1, runner.get_core1());

#ifdef ENABLE_TWO_PLAYERS_MODE
            if (runner.is_two_players_mode()) {
                ok |= save_framebuffer_png(game_framebuffer2, runner.get_core2());
            }
#endif

            if (ok) {
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
        case SDLK_SPACE:
            if (main.get_speed() == MainController::SPEED_UNLIMITED) {
                main.set_speed(MainController::SPEED_DEFAULT);
            } else {
                main.set_speed(MainController::SPEED_UNLIMITED);
            }
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
            Screen::Controllers controllers {runner, ui, menu.nav_controller, main, debugger};
#else
            Screen::Controllers controllers {runner, ui, menu.nav_controller, main};
#endif
            menu.screen_stack.push(std::make_unique<GameMainScreen>(Context {controllers, {0xF0}}));
            redraw();
        }
        default:
            runner.get_core1().send_key(event.key.key, Joypad::KeyState::Pressed);
#ifdef ENABLE_TWO_PLAYERS_MODE
            if (runner.is_two_players_mode()) {
                runner.get_core2().send_key(event.key.key, Joypad::KeyState::Pressed);
            }
#endif
            break;
        }
        break;
    }
    case SDL_EVENT_KEY_UP: {
        runner.get_core1().send_key(event.key.key, Joypad::KeyState::Released);
#ifdef ENABLE_TWO_PLAYERS_MODE
        if (runner.is_two_players_mode()) {
            runner.get_core2().send_key(event.key.key, Joypad::KeyState::Released);
        }
#endif
        break;
    }
    case SDL_EVENT_DROP_FILE: {
#ifdef ENABLE_TWO_PLAYERS_MODE
        if (!runner.is_two_players_mode()) {
            runner.get_core1().load_rom(event.drop.data);
        } else {
            // Drag & Drop to load ROM while playing is not supported with dual screen at the moment
        }
#else
        runner.get_core1().load_rom(event.drop.data);
#endif
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
    uint32_t text_color = ui.get_current_appearance().accent;

    uint32_t* overlay_texture_buffer = lock_texture(game_overlay_texture);

    clear_texture(overlay_texture_buffer, ui.get_width() * ui.get_height());

    if (popup.visible) {
        draw_text(overlay_texture_buffer, ui.get_width(), popup.text, 4, Specs::Display::HEIGHT - 12, text_color);
    }

    if (fps.visible) {
        const std::string fps_str = std::to_string(fps.display_count);
        draw_text(overlay_texture_buffer, ui.get_width(), fps_str, ui.get_width() - 4 - fps_str.size() * 8, 4,
                  text_color);
    }

    if (const int32_t speed {main.get_speed()}; speed != 0) {
        std::string speed_text =
            speed == MainController::SPEED_UNLIMITED
                ? ">>"
                : (speed > MainController::SPEED_DEFAULT ? "x" : "/") + std::to_string((uint32_t)(pow2(abs(speed))));
        draw_text(overlay_texture_buffer, ui.get_width(), speed_text, 4, 4, text_color);
    }

    unlock_texture(game_overlay_texture);
}

void GameScreen::render_game_texture() {
    SDL_FRect dest_rect {0, 0, static_cast<float>(Specs::Display::WIDTH * ui.get_scaling()),
                         static_cast<float>(Specs::Display::HEIGHT * ui.get_scaling())};

    SDL_RenderTexture(renderer, game_texture1, nullptr, &dest_rect);
#ifdef ENABLE_TWO_PLAYERS_MODE
    if (game_texture2) {
        dest_rect.x += static_cast<float>(Specs::Display::WIDTH * ui.get_scaling());
        SDL_RenderTexture(renderer, game_texture2, nullptr, &dest_rect);
    }
#endif
}

bool GameScreen::is_in_menu() const {
    return menu.screen_stack.top;
}

void GameScreen::on_scaling_filter_changed(UiController::ScalingFilter filter) const {
    SDL_ScaleMode texture_scale_mode = scaling_filter_to_texture_scale_mode(filter);

    SDL_SetTextureScaleMode(game_texture1, texture_scale_mode);
#ifdef ENABLE_TWO_PLAYERS_MODE
    if (runner.is_two_players_mode()) {
        SDL_SetTextureScaleMode(game_texture2, texture_scale_mode);
    }
#endif
    SDL_SetTextureScaleMode(game_overlay_texture, texture_scale_mode);
}
