#include "profiler.h"

Profiler::Profiler(ICore &core, IGameBoy &gb) : core(core), gameboy(gb), result() {
    startTime = std::chrono::high_resolution_clock::now();
}

void Profiler::setMaxTicks(uint64_t ticks) {
    maxTicks = ticks;
}

void Profiler::frame() {
    ILCDIO &lcd = gameboy.getLCDIO();

    if (lcd.readLY() >= 144) {
        while (core.isOn() && lcd.readLY() != 0) {
            core.tick();
            if (isProfilingFinished()) {
                computeProfilerResult();
                return;
            }
        }
    }
    while (core.isOn() && lcd.readLY() < 144) {
        core.tick();
        if (isProfilingFinished()) {
            computeProfilerResult();
            return;
        }
    }
}

void Profiler::tick() {
    core.tick();
}

ProfilerResult Profiler::getProfilerResult() const {
    return result;
}

bool Profiler::isProfilingFinished() const {
    if (maxTicks)
        return gameboy.getClock().getTicks() >= *maxTicks;
    return false;
}

void Profiler::computeProfilerResult() {
    result.executionTime = std::chrono::high_resolution_clock::now() - startTime;
    result.ticks = gameboy.getClock().getTicks();
}
