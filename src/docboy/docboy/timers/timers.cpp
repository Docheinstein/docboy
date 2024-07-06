#include "docboy/timers/timers.h"

#include "docboy/bootrom/helpers.h"
#include "docboy/interrupts/interrupts.h"

/* TIMA overflow timing example.
 *   t                  CPU                              Timer
 * CPU   |
 * Timer |                                      TIMA overflows -> Pending
 *
 * CPU   | [writing TIMA aborts PendingReload]
 *       | [writing TMA is considered]
 * Timer |                                      Pending -> Reload (TIMA = TMA, raise interrupt)
 *
 * CPU   | [writing TIMA is ignored]
 *       | [writing TMA writes TIMA too]
 * DMA   |                                      Reload -> None
 */

Timers::Timers(Interrupts& interrupts) :
    interrupts {interrupts} {
}

void Timers::tick_t3() {
    handle_pending_tima_reload();
    set_div(div + 4);
}

void Timers::save_state(Parcel& parcel) const {
    parcel.write_uint16(div);
    parcel.write_uint8(tima);
    parcel.write_uint8(tma);
    parcel.write_uint8(tac);
    parcel.write_uint8(tima_state);
    parcel.write_bool(last_div_bit_and_tac_enable);
}

void Timers::load_state(Parcel& parcel) {
    div = parcel.read_uint16();
    tima = parcel.read_uint8();
    tma = parcel.read_uint8();
    tac = parcel.read_uint8();
    tima_state = parcel.read_uint8();
    last_div_bit_and_tac_enable = parcel.read_bool();
}

void Timers::reset() {
    div = if_bootrom_else(0x0008, 0xABCC); // [mooneye/boot_div-dmgABCmgb.gb]
    tima = 0;
    tma = 0;
    tac = 0b11111000;

    tima_state = TimaReloadState::None;
    last_div_bit_and_tac_enable = false;
}

uint8_t Timers::read_div() const {
    const uint8_t div_high = div >> 8;
#ifdef ENABLE_DEBUGGER
    DebuggerMemoryWatcher::notify_read(Specs::Registers::Timers::DIV, div_high);
#endif
    return div_high;
}

void Timers::write_div(uint8_t value) {
#ifdef ENABLE_DEBUGGER
    uint8_t old_value = read_div();
#endif

    set_div(0);

#ifdef ENABLE_DEBUGGER
    DebuggerMemoryWatcher::notify_write(Specs::Registers::Timers::DIV, old_value, value);
#endif
}

void Timers::write_tima(uint8_t value) {
    // Writing to TIMA in the same cycle it has been reloaded is ignored
    if (tima_state != TimaReloadState::Reload) {
        tima = value;
        tima_state = TimaReloadState::None;
    }
}

void Timers::write_tma(uint8_t value) {
    // Writing TMA in the same cycle TIMA has been reloaded writes TIMA too
    tma = value;
    if (tima_state == TimaReloadState::Reload) {
        tima = value;
    }
}

void Timers::write_tac(uint8_t value) {
    tac = 0b11111000 | value;
    on_falling_edge_inc_tima();
}

inline void Timers::set_div(uint16_t value) {
    div = value;
    on_falling_edge_inc_tima();
}

inline void Timers::inc_tima() {
    // When TIMA overflows, TMA is reloaded in TIMA: but it is delayed by 1 m-cycle
    if (++tima == 0) {
        tima_state = TimaReloadState::Pending;
    }
}

inline void Timers::on_falling_edge_inc_tima() {
    // TIMA is incremented if (DIV bit selected by TAC && TAC enable)
    // was true and now it's false (on falling edge)
    const bool tac_div_bit = test_bit(div, Specs::Timers::TAC_DIV_BITS_SELECTOR[keep_bits<2>(tac)]);
    const bool tac_enable = test_bit<Specs::Bits::Timers::TAC::ENABLE>(tac);
    const bool div_bit_and_tac_enable = tac_div_bit && tac_enable;
    if (last_div_bit_and_tac_enable > div_bit_and_tac_enable) {
        inc_tima();
    }
    last_div_bit_and_tac_enable = div_bit_and_tac_enable;
}

inline void Timers::handle_pending_tima_reload() {
    if (tima_state != TimaReloadState::None) {
        if (--tima_state == TimaReloadState::Reload) {
            tima = (uint8_t)tma;
            interrupts.raise_Interrupt<Interrupts::InterruptType::Timer>();
        }
    }
}