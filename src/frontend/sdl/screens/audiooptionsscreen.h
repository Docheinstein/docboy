#ifndef AUDIOOPTIONSSCREEN
#define AUDIOOPTIONSSCREEN

#include <optional>

#include "components/menu/menu.h"
#include "screens/menuscreen.h"
#include "screens/screen.h"

class AudioOptionsScreen : public MenuScreen {
public:
    explicit AudioOptionsScreen(Context context);

    void redraw() override;

private:
    struct {
        MenuItem* audio_enabled {};
        MenuItem* volume {};
        struct {
            MenuItem* enabled_label {};
            MenuItem* enabled {};
            MenuItem* max_latency_label {};
            MenuItem* max_latency {};
            MenuItem* moving_average_factor_label {};
            MenuItem* moving_average_factor {};
            MenuItem* max_pitch_slack_factor_label {};
            MenuItem* max_pitch_slack_factor {};
        } dynamic_sample_rate_control;
    } items {};

    void on_change_audio_enabled();

    void on_decrease_volume();
    void on_increase_volume();

    void on_change_dynamic_sample_rate_control();

    void on_decrease_max_latency();
    void on_increase_max_latency();

    void on_decrease_dynamic_sample_rate_control_moving_average_factor();
    void on_increase_dynamic_sample_rate_control_moving_average_factor();

    void on_decrease_dynamic_sample_rate_control_max_pitch_slack();
    void on_increase_dynamic_sample_rate_control_max_pitch_slack();
};

#endif // AUDIOOPTIONSSCREEN
