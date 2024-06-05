#ifndef TIMERS_H
#define TIMERS_H

#include "docboy/common/macros.h"
#include "docboy/common/specs.h"
#include "docboy/memory/byte.h"

#include "utils/parcel.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/memsniffer.h"
#endif

class InterruptsIO;

class TimersIO {
public:
    DEBUGGABLE_CLASS()

    explicit TimersIO(InterruptsIO& interrupts);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    uint8_t read_div() const {
        const uint8_t div_high = div >> 8;
#ifdef ENABLE_DEBUGGER
        DebuggerMemorySniffer::notify_memory_read(Specs::Registers::Timers::DIV, div_high);
#endif
        return div_high;
    }

    void write_div(uint8_t value);
    void write_tima(uint8_t value);
    void write_tma(uint8_t value);
    void write_tac(uint8_t value);

    uint16_t div {};
    byte tima {make_byte(Specs::Registers::Timers::TIMA)};
    byte tma {make_byte(Specs::Registers::Timers::TMA)};
    byte tac {make_byte(Specs::Registers::Timers::TAC)};

protected:
    struct TimaReloadState {
        using Type = uint8_t;
        static constexpr Type Pending = 2;
        static constexpr Type Reload = 1;
        static constexpr Type None = 0;
    };

    void set_div(uint16_t value);
    void inc_tima();
    void on_falling_edge_inc_tima();
    void handle_pending_tima_reload();

    InterruptsIO& interrupts;

    TimaReloadState::Type tima_state {};
    bool last_div_bit_and_tac_enable {};
};

class Timers : public TimersIO {
public:
    explicit Timers(InterruptsIO& interrupts);

    void tick_t3();
};

#endif // TIMERS_H