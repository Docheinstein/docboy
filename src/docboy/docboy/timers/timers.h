#ifndef TIMERS_H
#define TIMERS_H

#include "docboy/bootrom/macros.h"
#include "docboy/memory/byte.hpp"
#include "docboy/shared/macros.h"
#include "docboy/shared/specs.h"
#include "utils/parcel.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/memsniffer.h"
#endif

class InterruptsIO;

class TimersIO {
public:
    DEBUGGABLE_CLASS()

    explicit TimersIO(InterruptsIO& interrupts);

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

    void reset();

    [[nodiscard]] uint8_t readDIV() const {
        const uint8_t DIVh = DIV >> 8;
        IF_DEBUGGER(DebuggerMemorySniffer::notifyMemoryRead(Specs::Registers::Timers::DIV, DIVh));
        return DIVh;
    }

    void writeDIV(uint8_t value);
    void writeTIMA(uint8_t value);
    void writeTMA(uint8_t value);
    void writeTAC(uint8_t value);

    uint16_t DIV {};
    byte TIMA {make_byte(Specs::Registers::Timers::TIMA)};
    byte TMA {make_byte(Specs::Registers::Timers::TMA)};
    byte TAC {make_byte(Specs::Registers::Timers::TAC)};

protected:
    struct TimaReloadState {
        using Type = uint8_t;
        static constexpr Type Pending = 2;
        static constexpr Type Reload = 1;
        static constexpr Type None = 0;
    };

    void setDIV(uint16_t value);
    void incTIMA();
    void onFallingEdgeIncTima();
    void handlePendingTimaReload();

    InterruptsIO& interrupts;

    TimaReloadState::Type timaState {};
    bool lastDivBitAndTacEnable {};
};

class Timers : public TimersIO {
public:
    explicit Timers(InterruptsIO& interrupts);

    void tick_t3();
};

#endif // TIMERS_H