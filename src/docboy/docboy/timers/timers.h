#ifndef TIMERS_H
#define TIMERS_H

#include "docboy/common/macros.h"
#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/memwatcher.h"
#endif

class Interrupts;

class Timers {
public:
    DEBUGGABLE_CLASS()

    explicit Timers(Interrupts& interrupts);

    void tick_t3();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    uint8_t read_div() const;

    void write_div(uint8_t value);
    void write_tima(uint8_t value);
    void write_tma(uint8_t value);
    void write_tac(uint8_t value);

    uint16_t div16 {};
    UInt8 tima {make_uint8(Specs::Registers::Timers::TIMA)};
    UInt8 tma {make_uint8(Specs::Registers::Timers::TMA)};
    UInt8 tac {make_uint8(Specs::Registers::Timers::TAC)};

private:
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

    Interrupts& interrupts;

    TimaReloadState::Type tima_state {};
    bool last_div_bit_and_tac_enable {};
};

#endif // TIMERS_H