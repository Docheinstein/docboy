#ifndef PROFILER_H
#define PROFILER_H

#include "core/core.h"
#include "core/gameboy.h"
#include <chrono>

struct ProfilerResult {
    std::chrono::high_resolution_clock::duration executionTime;
    uint64_t ticks;
};

class Profiler {
public:
    explicit Profiler(ICore& core, IGameBoy& gb);
    void setMaxTicks(uint64_t maxTicks);

    void frame();
    void tick();

    [[nodiscard]] bool isProfilingFinished() const;
    [[nodiscard]] ProfilerResult getProfilerResult() const;

private:
    void computeProfilerResult();

    ICore& core;
    IGameBoy& gameboy;
    std::chrono::high_resolution_clock::time_point startTime;
    std::optional<uint64_t> maxTicks;

    ProfilerResult result;
};

#endif // PROFILER_H