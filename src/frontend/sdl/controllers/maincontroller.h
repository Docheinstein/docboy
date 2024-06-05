#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <chrono>
#include <cstdint>

class MainController {
public:
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

private:
    static constexpr std::chrono::nanoseconds DEFAULT_FRAME_TIME {1000000000LU * Specs::Ppu::DOTS_PER_FRAME /
                                                                  Specs::Frequencies::CLOCK};

    int32_t speed {};
    std::chrono::high_resolution_clock::duration frame_time {DEFAULT_FRAME_TIME};
    bool quitting {};
};

#endif // MAINCONTROLLER_H
