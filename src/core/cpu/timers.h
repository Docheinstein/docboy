#ifndef TIMERS_H
#define TIMERS_H

#include "core/clock/clockable.h"
#include "core/io/interrupts.h"
#include "core/io/timers.h"
#include <cstddef>
#include <cstdint>

class Timers : public ITimersIO, public IClockable {
public:
    explicit Timers(IInterruptsIO& interrupts);

    void tick() override;

    [[nodiscard]] uint8_t readDIV() const override;
    void writeDIV(uint8_t value) override;

    [[nodiscard]] uint8_t readTIMA() const override;
    void writeTIMA(uint8_t value) override;

    [[nodiscard]] uint8_t readTMA() const override;
    void writeTMA(uint8_t value) override;

    [[nodiscard]] uint8_t readTAC() const override;
    void writeTAC(uint8_t value) override;

private:
    IInterruptsIO& interrupts;

    uint8_t DIV;
    uint8_t TIMA;
    uint8_t TMA;
    uint8_t TAC;

    uint32_t divTicks;
    uint32_t timaTicks;
};

#endif // TIMERS_H