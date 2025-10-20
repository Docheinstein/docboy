#include "docboy/timers/timers.h"

#include "docboy/bootrom/helpers.h"
#include "docboy/interrupts/interrupts.h"

#include "utils/parcel.h"

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

void Timers::tick() {
    handle_pending_tima_reload();
    set_div(div16 + 4);
}

void Timers::save_state(Parcel& parcel) const {
    PARCEL_WRITE_UINT16(parcel, div16);
    PARCEL_WRITE_UINT8(parcel, tima);
    PARCEL_WRITE_UINT8(parcel, tma);
    PARCEL_WRITE_BOOL(parcel, tac.enable);
    PARCEL_WRITE_UINT8(parcel, tac.clock_selector);
    PARCEL_WRITE_UINT8(parcel, tima_state);
    PARCEL_WRITE_BOOL(parcel, last_div_bit_and_tac_enable);
}

void Timers::load_state(Parcel& parcel) {
    div16 = parcel.read_uint16();
    tima = parcel.read_uint8();
    tma = parcel.read_uint8();
    tac.enable = parcel.read_bool();
    tac.clock_selector = parcel.read_uint8();
    tima_state = parcel.read_uint8();
    last_div_bit_and_tac_enable = parcel.read_bool();
}

void Timers::reset() {
#ifdef ENABLE_CGB
    // TODO: what's the starting div16 with bootrom on CGB?
    div16 = if_bootrom_else(0x0008, 0x1EA0);
#else
    div16 = if_bootrom_else(0x0008, 0xABCC); // [mooneye/boot_div-dmgABCmgb.gb]
#endif
    tima = 0;
    tma = 0;
    tac.enable = false;
    tac.clock_selector = 0;

    tima_state = TimaReloadState::None;
    last_div_bit_and_tac_enable = false;
}

uint8_t Timers::read_div() const {
    const uint8_t div_high = get_byte<1>(div16);
#ifdef ENABLE_DEBUGGER
    DebuggerMemoryWatcher::notify_read(Specs::Registers::Timers::DIV);
#endif
    return div_high;
}

void Timers::write_div(uint8_t value) {
#ifdef ENABLE_DEBUGGER
    DebuggerMemoryWatcher::notify_write(Specs::Registers::Timers::DIV);
#endif

    set_div(0);
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

uint8_t Timers::read_tac() const {
    return 0b11111000 | tac.enable << Specs::Bits::Timers::TAC::ENABLE |
           tac.clock_selector << Specs::Bits::Timers::TAC::CLOCK_SELECTOR;
}

void Timers::write_tac(uint8_t value) {
    tac.enable = test_bit<Specs::Bits::Timers::TAC::ENABLE>(value);
    tac.clock_selector = keep_bits_range<Specs::Bits::Timers::TAC::CLOCK_SELECTOR>(value);
    on_falling_edge_inc_tima();
}

void Timers::set_div(uint16_t value) {
    div16 = value;
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
    const bool tac_div_bit = test_bit(div16, Specs::Timers::TAC_DIV_BITS_SELECTOR[tac.clock_selector]);
    const bool div_bit_and_tac_enable = tac_div_bit && tac.enable;
    if (last_div_bit_and_tac_enable > div_bit_and_tac_enable) {
        inc_tima();
    }
    last_div_bit_and_tac_enable = div_bit_and_tac_enable;
}

inline void Timers::handle_pending_tima_reload() {
    if (tima_state != TimaReloadState::None) {
        if (--tima_state == TimaReloadState::Reload) {
            tima = (uint8_t)tma;
            interrupts.raise_interrupt<Interrupts::InterruptType::Timer>();
        }
    }
}