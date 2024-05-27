#ifndef GAMESCREEN_H
#define GAMESCREEN_H

#include "controllers/navcontroller.h"
#include "docboy/lcd/lcd.h"
#include "screen.h"
#include "screenstack.h"
#include <chrono>

struct SDL_Texture;

class GameScreen : public Screen {
public:
    explicit GameScreen(Context context);

    void redraw() override;
    void render() override;
    void handleEvent(const SDL_Event& event) override;

private:
    void drawPopup(const std::string& str);

    void redrawOverlay();

    bool isInMenu() const;

    SDL_Texture* gameTexture {};
    SDL_Texture* gameOverlayTexture {};

    const Lcd::PixelRgb565* gameFramebuffer {};

    struct {
        ScreenStack screenStack {};
        NavController navController {screenStack};
    } menu;

    struct {
        bool visible {};
        std::chrono::high_resolution_clock::time_point deadline {};
        std::string text {};
    } popup {};

    struct {
        bool visible {};
        std::chrono::high_resolution_clock::time_point lastSample {};
        uint32_t count {};
        uint32_t displayCount {};
    } fps;
};

#endif // GAMESCREEN_H
