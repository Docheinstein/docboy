#include "docboy/speedswitch/speedswitchcontroller.h"

#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/parcel.h"

SpeedSwitchController::SpeedSwitchController(SpeedSwitch& speed_switch, Timers& timers, bool& halted) :
    speed_switch {speed_switch},
    timers {timers},
    halted {halted} {
}

void SpeedSwitchController::switch_speed() {
    // Block CPU.
    switching_speed = true;
    halted = true;

    // TODO: are KEY1 changed now, or at the end of speed switch?
    speed_switch.key1.switch_speed = false;
    speed_switch.key1.current_speed = !speed_switch.key1.current_speed;

    if (speed_switch.key1.current_speed) {
        speed_switch_countdown = 16386 * 2;
    } else {
        speed_switch_countdown = 16386 * 4 - 4;
    }

    // Timer is blocked for 2 M-Cycles.
    // TODO: figure out precise timing of block (start or end of speed switch)?
    speed_switch_timers_block_countdown = 4;
    timers_blocked = true;
}

void SpeedSwitchController::tick_speed_switch() {
    ASSERT(speed_switch_countdown);

    if (--speed_switch_countdown == 0) {
        // Unblock CPU.
        switching_speed = false;
        halted = false;
    }

    if (speed_switch_timers_block_countdown) {
        if (--speed_switch_timers_block_countdown == 0) {
            // DIV is reset (indeed DIV's falling edge may increase TIMA).
            // Quirk: it seems that there is a window of an additional DIV tick for
            // increase TIMA on falling edge when TIMA runs at 262kHz.
            // What really happens inside is still obscure to me.
            // Keeping the last four bits of DIV and allowing it to tick before
            // it's reset has been the most reasonable way to pass the test roms
            // that came to my mind.
            // TODO: what really happens here?
            timers.set_div(keep_bits<4>(timers.div16));
            timers.tick();
            timers.set_div(0);

            // Unblock timers.
            timers_blocked = false;
        }
    }
}

void SpeedSwitchController::save_state(Parcel& parcel) const {
    parcel.write_uint32(speed_switch_countdown);
    parcel.write_bool(switching_speed);
    parcel.write_uint8(speed_switch_timers_block_countdown);
    parcel.write_bool(timers_blocked);
}

void SpeedSwitchController::load_state(Parcel& parcel) {
    speed_switch_countdown = parcel.read_uint32();
    switching_speed = parcel.read_bool();
    switching_speed = parcel.read_uint8();
    timers_blocked = parcel.read_bool();
}

void SpeedSwitchController::reset() {
    speed_switch_countdown = 0;
    switching_speed = false;
    speed_switch_timers_block_countdown = 0;
    timers_blocked = false;
}
