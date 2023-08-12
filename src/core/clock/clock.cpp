#include "clock.h"
#include "utils/log.h"
#include <algorithm>

Clock::Clock() :
    period(),
    ticks() {
}

void Clock::loadState(IReadableState& state) {
    ticks = state.readUInt64();
}

void Clock::saveState(IWritableState& state) {
    state.writeUInt64(ticks);
}

void Clock::tick() {
    // Wait until next tick
    std::chrono::high_resolution_clock::time_point now;
    do {
        now = std::chrono::high_resolution_clock::now();
    } while (now < nextTickTime);

    if (!ticks)
        // First tick
        nextTickTime = now + period;
    else
        // Already running
        nextTickTime += period;

    // Update devices needing tick
    for (const auto& clk : clockables) {
        if (ticks % clk.period == 0)
            clk.clockable->tick();
    }

    ++ticks;
}

uint64_t Clock::getTicks() const {
    return ticks;
}

void Clock::updatePeriods() {
    // Update periods based on the frequency of the device with the highest frequency
    auto it = std::max_element(clockables.begin(), clockables.end(), [](const Clockable& d1, const Clockable& d2) {
        return d1.frequency < d2.frequency;
    });
    uint64_t max_freq = it->frequency;
    period = std::chrono::nanoseconds(1000000000 / max_freq);
    for (auto& clk : clockables) {
        uint64_t clk_period = max_freq / clk.frequency;
        if (max_freq % clk.frequency != 0) {
            WARN() << "Device frequency (" << clk.frequency
                   << ") does not divide "
                      "clock frequency ("
                   << max_freq << "); expecting bad accuracy";
        }
        clk.period = clk_period;
    }
}

void Clock::attach(IClockable* clockable, uint64_t frequency) {
    Clockable d = {.clockable = clockable, .frequency = frequency, .period = 1};
    clockables.push_back(d);
    updatePeriods();
}

void Clock::detach(IClockable* clockable) {
    auto it = std::remove_if(clockables.begin(), clockables.end(), [clockable](const Clockable& clk) {
        return clk.clockable == clockable;
    });
    clockables.erase(it, clockables.end());
    updatePeriods();
}

void Clock::reattach(IClockable* clockable, uint64_t frequency) {
    for (auto& clk : clockables) {
        if (clk.clockable == clockable) {
            clk.frequency = frequency;
            break;
        }
    }
    updatePeriods();
}