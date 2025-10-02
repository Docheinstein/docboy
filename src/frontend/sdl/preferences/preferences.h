#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <string>

#include "SDL3/SDL.h"

#include "controllers/uicontroller.h"

struct Preferences {
    struct Keys {
        SDL_Keycode a {};
        SDL_Keycode b {};
        SDL_Keycode start {};
        SDL_Keycode select {};
        SDL_Keycode left {};
        SDL_Keycode up {};
        SDL_Keycode right {};
        SDL_Keycode down {};
    };

#ifndef ENABLE_CGB
    UiController::LcdAppearance dmg_palette {};
#endif

    struct {
        Keys player1 {};
#ifdef ENABLE_TWO_PLAYERS_MODE
        Keys player2 {};
#endif
    } keys {};
    uint32_t scaling {};
    UiController::ScalingFilter scaling_filter {};
    int x {};
    int y {};
#ifdef ENABLE_AUDIO
    bool audio {};
#ifdef ENABLE_TWO_PLAYERS_MODE
    uint8_t audio_player_source {};
#endif
    uint8_t volume {};
    bool high_pass_filter {};
    struct {
        bool enabled {};
        uint32_t max_latency {};
        double moving_average_factor {};
        double max_pitch_slack_factor {};
    } dynamic_sample_rate {};
#endif

#ifdef ENABLE_TWO_PLAYERS_MODE
    bool serial_link {};
#endif
};

std::string get_preferences_path();

void read_preferences(const std::string& path, Preferences& p);
void write_preferences(const std::string& path, const Preferences& p);

#endif // PREFERENCES_H
