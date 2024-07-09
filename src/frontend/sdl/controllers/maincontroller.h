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
        audio = enabled;
    }

    bool is_audio_enabled() const {
        return audio;
    }

    void set_volume(uint8_t vol) {
        ASSERT(vol <= 100);
        volume = vol;
        if (volume_changed_callback) {
            volume_changed_callback(volume);
        }
    }

    uint8_t get_volume() const {
        return volume;
    }

    void set_volume_changed_callback(std::function<void(uint8_t volume)>&& callback) {
        volume_changed_callback = std::move(callback);
    }
#endif

private:
    static constexpr std::chrono::nanoseconds DEFAULT_FRAME_TIME {1000000000LLU * Specs::Ppu::DOTS_PER_FRAME /
                                                                  Specs::Frequencies::CLOCK};

    int32_t speed {};
    std::chrono::high_resolution_clock::duration frame_time {DEFAULT_FRAME_TIME};
    bool quitting {};

#ifdef ENABLE_AUDIO
    bool audio {true};
    uint8_t volume {100};
    std::function<void(uint8_t)> volume_changed_callback {};
#endif
};

#endif // MAINCONTROLLER_H
