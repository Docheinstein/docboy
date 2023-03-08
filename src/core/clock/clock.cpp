#include "clock.h"
#include <algorithm>
#include "utils/log.h"


Clock::Clock(std::initializer_list<std::pair<IClockable *, uint64_t>> devs)
        : period(), ticks() {
    // Find out max frequency and express every other device's period in terms of that frequency

    auto it = std::max_element(devs.begin(), devs.end(), [](const auto &d1, const auto &d2) {
       return d1.second < d2.second;
    });
    uint64_t max_freq = it->second;
    period = std::chrono::nanoseconds(1000000000 / max_freq);
    for (const auto &dev : devs) {
        uint64_t dev_period = max_freq / dev.second;
        if (max_freq % dev.second != 0) {
            WARN() <<   "Device frequency (" << dev.second << ") does not divide "
                        "clock frequency (" << max_freq << "); expecting bad accuracy";
        }
        Device d {
            .device = dev.first,
            .period = dev_period
        };
        devices.push_back(d);
    }
}

void Clock::tick() {
    // Wait until next tick
    std::chrono::high_resolution_clock::time_point now;
    do {
        now = std::chrono::high_resolution_clock::now();
    } while (now < nextTickTime);
    nextTickTime = now + period;

    // Update devices needing tick
    for (const auto &dev : devices) {
        if (ticks % dev.period == 0)
            dev.device->tick();
    }

    ++ticks;
}

uint64_t Clock::getTicks() const {
    return ticks;
}


