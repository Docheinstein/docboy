#include "docboy/stop/stopcontroller.h"

#include "docboy/joypad/joypad.h"
#include "docboy/lcd/lcd.h"
#include "docboy/ppu/ppu.h"
#include "docboy/timers/timers.h"

StopController::StopController(Joypad& joypad, Timers& timers, Ppu& ppu, Lcd& lcd) :
    joypad {joypad},
    timers {timers},
    ppu {ppu},
    lcd {lcd} {
}

void StopController::stop() {
    ASSERT(!stopped);
    ASSERT(!requested);

    // Request STOP (it will be stopped at the end of the M-Cycle)
    requested = true;
}

void StopController::stopped_tick() {
    ASSERT(stopped);
    ASSERT(!requested);

    // STOP mode behaves differently while in DMG and CGB.
#ifdef ENABLE_CGB
    // In CGB, PPU is ticked even while stopped (it continues to render).
    ppu.tick();
#else
    // In DMG, PPU is ticked for a while and then LCD is turned off.
    // Note that in this case PPU retains its state and resumes from there
    // when stop mode is exited.
    if (ppu_shutdown_countdown > 0) {
        ppu.tick();
        if (--ppu_shutdown_countdown == 0) {
            // LCD is cleared, but PPU retains its state (it's not reset).
            lcd.clear();
        }
    }
#endif
}

void StopController::stopped_tock() {
    ASSERT(stopped);
    ASSERT(!requested);

    stopped_tick();

#ifndef ENABLE_CGB
    // For the sake of simplicity, I assume STOP mode can't
    // be exited during initial PPU advance.
    // TODO: investigate further
    if (ppu_shutdown_countdown == 0)
#endif
    {
        stopped = joypad.read_keys() == bitmask<4>;
    }

    if (!stopped) {
        timers.tick();

#ifdef ENABLE_CGB
        // Make PPU work again.
        ppu.enable_color_resolver();
#endif
    }
}

void StopController::enter_stop_mode() {
    ASSERT(!stopped);
    ASSERT(requested);

    // Enter STOP mode
    requested = false;
    stopped = true;

#ifndef ENABLE_CGB
    // In DMG, PPU is not stopped instantly.
    // This is shown by the fact that at the end of STOP mode
    // PPU state is different from the state it had before the STOP.
    // TODO: understand what really happens and whether the countdown is exact.
    ppu_shutdown_countdown = 60852; // TODO: both [60852, 60853] seems equally valid
#endif

    // DIV is reset
    timers.set_div(0);

#ifdef ENABLE_CGB
    // In CGB, PPU will push black pixels during the STOP mode
    // if the STOP has been triggered while PPU is on and not
    // in Pixel Transfer mode.
    if (ppu.lcdc.enable) {
        if (ppu.stat.mode != Specs::Ppu::Modes::PIXEL_TRANSFER) {
            ppu.disable_color_resolver();
        }
    }
#endif
}

void StopController::save_state(Parcel& parcel) const {
    parcel.write_bool(stopped);
    parcel.write_bool(requested);
#ifndef ENABLE_CGB
    parcel.write_uint16(ppu_shutdown_countdown);
#endif
}

void StopController::load_state(Parcel& parcel) {
    stopped = parcel.read_bool();
    requested = parcel.read_bool();
#ifndef ENABLE_CGB
    ppu_shutdown_countdown = parcel.read_uint16();
#endif
}

void StopController::reset() {
    stopped = false;
    requested = false;
#ifndef ENABLE_CGB
    ppu_shutdown_countdown = 0;
#endif
}
