#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <chrono>
#include <cstdint>

#include "utils/asserts.h"

class MainController {
public:
    static constexpr uint8_t VOLUME_MAX = 100;

    void set_speed(int32_t s) {
        speed = s;
        frame_time = std::chrono::nanoseconds {(uint64_t)(DEFAULT_FRAME_TIME.count() / pow2(speed))};
    }

    int32_t get_speed() const {
        return speed;
    }

    std::chrono::high_resolution_clock::duration get_frame_time() const {
        return frame_time;
    }

    void quit() {
        quitting = true;
    }

    bool should_quit() const {
        return quitting;
    }

#ifdef ENABLE_AUDIO
    void set_audio_enabled(bool enabled) {
        audio.enabled = enabled;
        if (audio.enabled_changed_callback) {
            audio.enabled_changed_callback(audio.enabled);
        }
    }

    bool is_audio_enabled() const {
        return audio.enabled;
    }

    void set_audio_enabled_changed_callback(std::function<void(uint8_t volume)>&& callback) {
        audio.enabled_changed_callback = std::move(callback);
    }

    void set_volume(uint8_t vol) {
        ASSERT(vol <= 100);
        audio.volume = vol;
        if (audio.volume_changed_callback) {
            audio.volume_changed_callback(audio.volume);
        }
    }

    uint8_t get_volume() const {
        return audio.volume;
    }

    void set_volume_changed_callback(std::function<void(uint8_t volume)>&& callback) {
        audio.volume_changed_callback = std::move(callback);
    }

    void set_dynamic_sample_rate_control_enabled(bool enabled) {
        audio.dynamic_sample_rate_control.enabled = enabled;
        if (audio.dynamic_sample_rate_control_settings_changed) {
            audio.dynamic_sample_rate_control_settings_changed();
        }
    }

    bool is_dynamic_sample_rate_control_enabled() const {
        return audio.dynamic_sample_rate_control.enabled;
    }

    void set_dynamic_sample_rate_control_max_latency(double ms) {
        audio.dynamic_sample_rate_control.max_latency = ms;
        if (audio.dynamic_sample_rate_control_settings_changed) {
            audio.dynamic_sample_rate_control_settings_changed();
        }
    }

    double get_dynamic_sample_rate_control_max_latency() const {
        return audio.dynamic_sample_rate_control.max_latency;
    }

    void set_dynamic_sample_rate_control_moving_average_factor(double factor) {
        audio.dynamic_sample_rate_control.moving_average_factor = factor;
        if (audio.dynamic_sample_rate_control_settings_changed) {
            audio.dynamic_sample_rate_control_settings_changed();
        }
    }

    double get_dynamic_sample_rate_control_moving_average_factor() const {
        return audio.dynamic_sample_rate_control.moving_average_factor;
    }

    void set_dynamic_sample_rate_control_max_pitch_slack_factor(double factor) {
        audio.dynamic_sample_rate_control.max_pitch_slack_factor = factor;
        if (audio.dynamic_sample_rate_control_settings_changed) {
            audio.dynamic_sample_rate_control_settings_changed();
        }
    }

    double get_dynamic_sample_rate_control_max_pitch_slack_factor() const {
        return audio.dynamic_sample_rate_control.max_pitch_slack_factor;
    }

    void set_dynamic_sample_rate_control_settings_changed(std::function<void()>&& callback) {
        audio.dynamic_sample_rate_control_settings_changed = std::move(callback);
    }
#endif

private:
    static constexpr std::chrono::nanoseconds DEFAULT_FRAME_TIME {1000000000LLU * Specs::Ppu::DOTS_PER_FRAME /
                                                                  Specs::Frequencies::CLOCK};

    int32_t speed {};
    std::chrono::high_resolution_clock::duration frame_time {DEFAULT_FRAME_TIME};
    bool quitting {};

#ifdef ENABLE_AUDIO
    struct {
        bool enabled {true};
        std::function<void(uint8_t)> enabled_changed_callback {};
        uint8_t volume {100};
        std::function<void(uint8_t)> volume_changed_callback {};
        struct {
            bool enabled {true};
            double max_latency {50};
            double moving_average_factor {0.005};
            double max_pitch_slack_factor {0.005};
        } dynamic_sample_rate_control;
        std::function<void()> dynamic_sample_rate_control_settings_changed {};
    } audio;
#endif
};

#endif // MAINCONTROLLER_H
