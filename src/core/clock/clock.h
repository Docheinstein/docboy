#ifndef CLOCK_H
#define CLOCK_H

#include "clockable.h"
#include "core/state/processor.h"
#include <chrono>
#include <cstdint>
#include <vector>

class IClock : public IClockable {
public:
    static constexpr uint64_t MAX_FREQUENCY = 9223372036854775808UL; // 2**63

    virtual void attach(IClockable* clockable, uint64_t frequency) = 0;
    virtual void detach(IClockable* device) = 0;
    virtual void reattach(IClockable* device, uint64_t frequency) = 0;

    [[nodiscard]] virtual uint64_t getTicks() const = 0;
};

class Clock : public IClock, public IStateProcessor {
public:
    Clock();

    void loadState(IReadableState& state) override;
    void saveState(IWritableState& state) override;

    void tick() override;

    void attach(IClockable* clockable, uint64_t frequency) override;
    void detach(IClockable* device) override;
    void reattach(IClockable* device, uint64_t frequency) override;

    [[nodiscard]] uint64_t getTicks() const override;

private:
    void updatePeriods();

    struct Clockable {
        IClockable* clockable;
        uint64_t frequency;
        uint64_t period;
    };

    std::vector<Clockable> clockables;
    std::chrono::high_resolution_clock::duration period;
    std::chrono::high_resolution_clock::time_point nextTickTime;
    uint64_t ticks;
};

#endif // CLOCK_H