#ifndef DEBUGGERTIMERS_H
#define DEBUGGERTIMERS_H

#include "core/cpu/timers.h"
#include "core/debugger/io/timers.h"

class DebuggableTimers : public Timers, public ITimersIODebug {
public:
    explicit DebuggableTimers(IInterruptsIO& interrupts);

    [[nodiscard]] uint8_t readDIV() const override;
    void writeDIV(uint8_t value) override;

    [[nodiscard]] uint8_t readTIMA() const override;
    void writeTIMA(uint8_t value) override;

    [[nodiscard]] uint8_t readTMA() const override;
    void writeTMA(uint8_t value) override;

    [[nodiscard]] uint8_t readTAC() const override;
    void writeTAC(uint8_t value) override;

    void setObserver(Observer* o) override;

private:
    Observer* observer;
};

#endif // DEBUGGERTIMERS_H