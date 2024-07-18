#include "screens/audiooptionsscreen.h"

#include <algorithm>

#include "components/menu/items.h"
#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"

#include "utils/strings.h"

#include "SDL3/SDL.h"

namespace {
constexpr uint8_t VOLUME_STEP = 10;
constexpr double MOVING_AVERAGE_FACTOR_STEP = 0.005;
constexpr double MAX_PITCH_FACTOR_STEP = 0.005;
constexpr double MAX_LATENCY_STEP = 1;
} // namespace

AudioOptionsScreen::AudioOptionsScreen(Context context) :
    MenuScreen {context} {

    items.audio_enabled = &menu.add_item(Spinner {[this] {
        on_change_audio_enabled();
    }});

    items.volume = &menu.add_item(Spinner {[this] {
                                               on_decrease_volume();
                                           },
                                           [this] {
                                               on_increase_volume();
                                           }});

    menu.add_item(Label {"Dynamic Rate Ctrl", MenuItem::Justification::Center});

    items.dynamic_sample_rate_control.enabled = &menu.add_item(Spinner {[this] {
                                                                            on_change_dynamic_sample_rate_control();
                                                                        },
                                                                        MenuItem::Justification::Center});

    items.dynamic_sample_rate_control.max_latency_label =
        &menu.add_item(Label {"Max Latency", MenuItem::Justification::Center});

    items.dynamic_sample_rate_control.max_latency = &menu.add_item(Spinner {[this] {
                                                                                on_decrease_max_latency();
                                                                            },
                                                                            [this] {
                                                                                on_increase_max_latency();
                                                                            },
                                                                            MenuItem::Justification::Center});

    items.dynamic_sample_rate_control.moving_average_factor_label =
        &menu.add_item(Label {"Moving Avg. Factor", MenuItem::Justification::Center});

    items.dynamic_sample_rate_control.moving_average_factor =
        &menu.add_item(Spinner {[this] {
                                    on_decrease_dynamic_sample_rate_control_moving_average_factor();
                                },
                                [this] {
                                    on_increase_dynamic_sample_rate_control_moving_average_factor();
                                },
                                MenuItem::Justification::Center});

    items.dynamic_sample_rate_control.max_pitch_slack_factor_label =
        &menu.add_item(Label {"Max Pitch Slack F.", MenuItem::Justification::Center});

    items.dynamic_sample_rate_control.max_pitch_slack_factor =
        &menu.add_item(Spinner {[this] {
                                    on_decrease_dynamic_sample_rate_control_max_pitch_slack();
                                },
                                [this] {
                                    on_increase_dynamic_sample_rate_control_max_pitch_slack();
                                },
                                MenuItem::Justification::Center});
}

void AudioOptionsScreen::redraw() {
    const bool dynamic_sample_rate_control_enabled = main.is_dynamic_sample_rate_control_enabled();
    items.audio_enabled->text = "Audio   " + std::string {main.is_audio_enabled() ? "Enabled" : "Disabled"};
    items.volume->text = "Volume  " + std::to_string(main.get_volume());

    items.dynamic_sample_rate_control.enabled->text =
        std::string {main.is_dynamic_sample_rate_control_enabled() ? "Enabled" : "Disabled"};

    items.dynamic_sample_rate_control.max_latency_label->visible = dynamic_sample_rate_control_enabled;
    items.dynamic_sample_rate_control.max_latency->visible = dynamic_sample_rate_control_enabled;
    items.dynamic_sample_rate_control.max_latency->text =
        to_string_with_precision<0>(main.get_dynamic_sample_rate_control_max_latency());

    items.dynamic_sample_rate_control.moving_average_factor_label->visible = dynamic_sample_rate_control_enabled;
    items.dynamic_sample_rate_control.moving_average_factor->visible = dynamic_sample_rate_control_enabled;
    items.dynamic_sample_rate_control.moving_average_factor->text =
        to_string_with_precision<3>(main.get_dynamic_sample_rate_control_moving_average_factor());

    items.dynamic_sample_rate_control.max_pitch_slack_factor_label->visible = dynamic_sample_rate_control_enabled;
    items.dynamic_sample_rate_control.max_pitch_slack_factor->visible = dynamic_sample_rate_control_enabled;
    items.dynamic_sample_rate_control.max_pitch_slack_factor->text =
        to_string_with_precision<3>(main.get_dynamic_sample_rate_control_max_pitch_slack_factor());

    menu.invalidate_layout();
    MenuScreen::redraw();
}

void AudioOptionsScreen::on_change_audio_enabled() {
    main.set_audio_enabled(!main.is_audio_enabled());
    redraw();
}

void AudioOptionsScreen::on_decrease_volume() {
    int32_t volume = std::max(main.get_volume() - VOLUME_STEP, 0);
    ASSERT(volume >= 0 && volume <= MainController::VOLUME_MAX);
    main.set_volume(volume);
    redraw();
}

void AudioOptionsScreen::on_increase_volume() {
    int32_t volume = std::min(main.get_volume() + VOLUME_STEP, static_cast<int>(MainController::VOLUME_MAX));
    ASSERT(volume >= 0 && volume <= MainController::VOLUME_MAX);
    main.set_volume(volume);
    redraw();
}

void AudioOptionsScreen::on_change_dynamic_sample_rate_control() {
    main.set_dynamic_sample_rate_control_enabled(!main.is_dynamic_sample_rate_control_enabled());
    redraw();
}

void AudioOptionsScreen::on_decrease_max_latency() {
    double ms = std::max(main.get_dynamic_sample_rate_control_max_latency() - MAX_LATENCY_STEP, 0.0);
    main.set_dynamic_sample_rate_control_max_latency(ms);
    redraw();
}

void AudioOptionsScreen::on_increase_max_latency() {
    double ms = std::min(main.get_dynamic_sample_rate_control_max_latency() + MAX_LATENCY_STEP, 1000.0);
    main.set_dynamic_sample_rate_control_max_latency(ms);
    redraw();
}

void AudioOptionsScreen::on_decrease_dynamic_sample_rate_control_moving_average_factor() {
    double f = std::max(main.get_dynamic_sample_rate_control_moving_average_factor() - MOVING_AVERAGE_FACTOR_STEP, 0.0);
    main.set_dynamic_sample_rate_control_moving_average_factor(f);
    redraw();
}

void AudioOptionsScreen::on_increase_dynamic_sample_rate_control_moving_average_factor() {
    double f = std::min(main.get_dynamic_sample_rate_control_moving_average_factor() + MOVING_AVERAGE_FACTOR_STEP, 1.0);
    main.set_dynamic_sample_rate_control_moving_average_factor(f);
    redraw();
}

void AudioOptionsScreen::on_decrease_dynamic_sample_rate_control_max_pitch_slack() {
    double f = std::max(main.get_dynamic_sample_rate_control_max_pitch_slack_factor() - MAX_PITCH_FACTOR_STEP, 0.0);
    main.set_dynamic_sample_rate_control_max_pitch_slack_factor(f);
    redraw();
}

void AudioOptionsScreen::on_increase_dynamic_sample_rate_control_max_pitch_slack() {
    double f = std::min(main.get_dynamic_sample_rate_control_max_pitch_slack_factor() + MAX_PITCH_FACTOR_STEP, 1.0);
    main.set_dynamic_sample_rate_control_max_pitch_slack_factor(f);
    redraw();
}
