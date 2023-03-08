#ifndef CLOCK_H
#define CLOCK_H

#include <cstdint>
#include <vector>
#include <initializer_list>
#include <chrono>
#include "clockable.h"

class Clock {
public:
    static constexpr uint64_t MAX_FREQUENCY = 9223372036854775808UL; // 2**63

    Clock(std::initializer_list<std::pair<IClockable *, uint64_t>> devices);
    void tick();

    [[nodiscard]] uint64_t getTicks() const;

private:
    struct Device {
        IClockable *device;
        uint64_t period;
    };

    std::vector<Device> devices;
    std::chrono::high_resolution_clock::duration period;
    std::chrono::high_resolution_clock::time_point nextTickTime;
    uint64_t ticks;
};

#endif // CLOCK_H