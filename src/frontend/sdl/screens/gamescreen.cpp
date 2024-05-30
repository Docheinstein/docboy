#include "gamescreen.h"
#include "SDL3/SDL.h"
#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/uicontroller.h"
#include "extra/img/imgmanip.h"
#include "extra/png/iopng.h"
#include "mainscreen.h"
#include "utils/io.h"
#include "utils/rompicker.h"

#ifdef ENABLE_DEBUGGER
#include "controllers/debuggercontroller.h"
#endif

GameScreen::GameScreen(Context context) :
    Screen(context),
    gameTexture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, Specs::Display::WIDTH,
                                  Specs::Display::HEIGHT)),
    gameOverlayTexture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                         Specs::Display::WIDTH, Specs::Display::HEIGHT)),
    gameFramebuffer {core.getFramebuffer()} {

    SDL_SetTextureScaleMode(gameTexture, SDL_SCALEMODE_NEAREST);

    SDL_SetTextureScaleMode(gameOverlayTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(gameOverlayTexture, SDL_BLENDMODE_BLEND);

    check(core.isRomLoaded());

    // Exit pause state (start emulation)
    core.setPaused(false);

    // Pause the emulation if menu is visible
    menu.screenStack.onScreenChanged = [this]() {
        core.setPaused(isInMenu());
    };
}

void GameScreen::redraw() {
    if (isInMenu()) {
        menu.screenStack.top->redraw();
        return;
    }

    redrawOverlay();
}

void GameScreen::render() {
    if (isInMenu()) {
        // Draw game framebuffer even in menu: it will be shown underneath the semi-transparent menu
        SDL_RenderTexture(renderer, gameTexture, nullptr, nullptr);
        menu.screenStack.top->render();
        return;
    }

    // Copy framebuffer to rendered texture
    copyToTexture(gameTexture, gameFramebuffer, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t));

    // Update FPS
    if (fps.visible) {
        fps.count++;
        const auto now = std::chrono::high_resolution_clock::now();
        if (now - fps.lastSample > std::chrono::nanoseconds(1000000000)) {
            fps.displayCount = fps.count;
            fps.lastSample = now;
            fps.count = 0;
            redrawOverlay();
        }
    }

    // Eventually remove expired popup
    if (popup.visible && std::chrono::high_resolution_clock::now() > popup.deadline) {
        popup.visible = false;
        redrawOverlay();
    }

    SDL_RenderTexture(renderer, gameTexture, nullptr, nullptr);
    SDL_RenderTexture(renderer, gameOverlayTexture, nullptr, nullptr);
}

void GameScreen::handleEvent(const SDL_Event& event) {
    if (isInMenu()) {
        menu.screenStack.top->handleEvent(event);
        return;
    }

    check(!core.isPaused());

    switch (event.type) {
    case SDL_EVENT_KEY_DOWN: {
        switch (event.key.keysym.sym) {
        case SDLK_F1:
            if (core.writeState())
                drawPopup("State saved");
            break;
        case SDLK_F2:
            if (core.loadState())
                drawPopup("State loaded");
            break;
        case SDLK_F11: {
            bool ok {};
            write_file((temp_directory_path() / core.getRom().with_extension("dat").filename()).string(),
                       gameFramebuffer, Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint16_t), &ok);
            if (ok)
                drawPopup("Framebuffer saved");
            break;
        }
        case SDLK_F12: {
            auto buffer_rgb888 =
                create_image_buffer(Specs::Display::WIDTH, Specs::Display::HEIGHT, ImageFormat::RGB888);
            convert_image(ImageFormat::RGB565, gameFramebuffer, ImageFormat::RGB888, buffer_rgb888.data(),
                          Specs::Display::WIDTH, Specs::Display::HEIGHT);
            if (save_png_rgb888((temp_directory_path() / core.getRom().with_extension("png").filename()).string(),
                                buffer_rgb888.data(), Specs::Display::WIDTH, Specs::Display::HEIGHT))
                drawPopup("Screenshot saved");
            break;
        }
        case SDLK_f:
            fps.visible = !fps.visible;
            fps.lastSample = std::chrono::high_resolution_clock::now();
            fps.displayCount = 0;
            fps.count = 0;
            redrawOverlay();
            break;
        case SDLK_q:
            main.setSpeed(main.getSpeed() - 1);
            redrawOverlay();
            break;
        case SDLK_w:
            main.setSpeed(main.getSpeed() + 1);
            redrawOverlay();
            break;
#ifdef ENABLE_DEBUGGER
        case SDLK_d:
            if (debugger.isDebuggerAttached()) {
                if (debugger.detachDebugger())
                    drawPopup("Debugger detached");
            } else {
                if (debugger.attachDebugger(true))
                    drawPopup("Debugger attached");
            }
            break;
#endif
        case SDLK_ESCAPE: {
            // Open menu with a semi-transparent background
            menu.screenStack.push(std::make_unique<MainScreen>(
                Context {{core, ui, menu.navController, main IF_DEBUGGER(COMMA debugger)}, {0xE0}}));
            redraw();
        }
        default:
            core.sendKey(event.key.keysym.sym, Joypad::KeyState::Pressed);
            break;
        }
        break;
    }
    case SDL_EVENT_KEY_UP: {
        core.sendKey(event.key.keysym.sym, Joypad::KeyState::Released);
        break;
    }
    case SDL_EVENT_DROP_FILE: {
        core.loadRom(event.drop.data);
        break;
    }
    }
}

void GameScreen::drawPopup(const std::string& str) {
    popup.visible = true;
    popup.deadline = std::chrono::high_resolution_clock::now() + std::chrono::seconds(2);
    popup.text = str;
    redrawOverlay();
}

void GameScreen::redrawOverlay() {
    uint32_t textColor = ui.getCurrentPalette().rgba8888.accent;

    clearTexture(gameOverlayTexture, Specs::Display::WIDTH * Specs::Display::HEIGHT);

    if (popup.visible) {
        drawText(gameOverlayTexture, popup.text, 4, Specs::Display::HEIGHT - 12, textColor, Specs::Display::WIDTH);
    }

    if (fps.visible) {
        const std::string fpsStr = std::to_string(fps.displayCount);
        drawText(gameOverlayTexture, fpsStr, Specs::Display::WIDTH - 4 - fpsStr.size() * 8, 4, textColor,
                 Specs::Display::WIDTH);
    }

    if (const int32_t speed {main.getSpeed()}; speed != 0) {
        drawText(gameOverlayTexture, (speed > 0 ? "x" : "/") + std::to_string((uint32_t)(pow2(abs(speed)))), 4, 4,
                 textColor, Specs::Display::WIDTH);
    }
}

bool GameScreen::isInMenu() const {
    return menu.screenStack.top;
}