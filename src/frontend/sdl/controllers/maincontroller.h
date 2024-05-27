#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include "utils/mathematics.h"
#include <chrono>
#include <cstdint>

class MainController {
public:
    void setSpeed(int32_t s) {
        speed = s;
        frameTime = std::chrono::nanoseconds {(uint64_t)(DEFAULT_FRAME_TIME.count() / pow2(speed))};
    }

    int32_t getSpeed() const {
        return speed;
    }

    std::chrono::high_resolution_clock::duration getFrameTime() const {
        return frameTime;
    }

    void quit() {
        quitting = true;
    }
    bool shouldQuit() const {
        return quitting;
    }

private:
    static constexpr std::chrono::nanoseconds DEFAULT_FRAME_TIME {1000000000LU * Specs::Ppu::DOTS_PER_FRAME /
                                                                  Specs::Frequencies::CLOCK};

    int32_t speed {};
    std::chrono::high_resolution_clock::duration frameTime {DEFAULT_FRAME_TIME};
    bool quitting {};
};

#endif // MAINCONTROLLER_H
