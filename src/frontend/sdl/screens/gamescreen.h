#ifndef GAMESCREEN_H
#define GAMESCREEN_H

#include <chrono>

#include "controllers/navcontroller.h"
#include "controllers/uicontroller.h"

#include "screens/screen.h"
#include "screens/screenstack.h"

#include "docboy/lcd/lcd.h"

struct SDL_Texture;

class GameScreen : public Screen {
public:
    explicit GameScreen(Context context);

    void redraw() override;
    void render() override;
    void handle_event(const SDL_Event& event) override;

private:
    void draw_popup(const std::string& str);

    void redraw_overlay();

    void render_game_texture();

    bool is_in_menu() const;

    void on_scaling_filter_changed(UiController::ScalingFilter filter) const;

    SDL_Texture* game_texture1 {};
#ifdef ENABLE_TWO_PLAYERS_MODE
    SDL_Texture* game_texture2 {};
#endif

    SDL_Texture* game_overlay_texture {};

    const PixelRgb565* game_framebuffer1 {};
#ifdef ENABLE_TWO_PLAYERS_MODE
    const PixelRgb565* game_framebuffer2 {};
#endif

    struct {
        ScreenStack screen_stack {};
        NavController nav_controller {screen_stack};
    } menu;

    struct {
        bool visible {};
        std::chrono::high_resolution_clock::time_point deadline {};
        std::string text {};
    } popup {};

    struct {
        bool visible {};
        std::chrono::high_resolution_clock::time_point last_sample {};
        uint32_t count {};
        uint32_t display_count {};
    } fps;
};

#endif // GAMESCREEN_H
