#include "docboy/stop/stopcontroller.h"
#include "docboy/joypad/joypad.h"
#include "docboy/lcd/lcd.h"
#include "docboy/timers/timers.h"

#include "utils/bits.h"
#include "utils/parcel.h"

StopController::StopController(bool& stopped, Joypad& joypad, Timers& timers, Lcd& lcd) :
    stopped {stopped},
    joypad {joypad},
    timers {timers},
    lcd {lcd} {
}

void StopController::stop() {
    ASSERT(!stopped);
    ASSERT(!requested);

    // Request STOP
    requested = true;
}

void StopController::enter_stop_mode() {
    ASSERT(!stopped);
    ASSERT(requested);

    // Enter STOP mode
    requested = false;
    stopped = true;

    // DIV is reset
    timers.div16 = 0;

    // LCD is cleared
    lcd.clear();
}

void StopController::eventually_exit_stop_mode() {
    ASSERT(stopped);
    ASSERT(!requested);

    // Exit STOP mode if there's joypad input
    stopped = keep_bits<4>(joypad.read_p1()) == bitmask<4>;
}

void StopController::save_state(Parcel& parcel) const {
    parcel.write_bool(stopped);
    parcel.write_bool(requested);
}

void StopController::load_state(Parcel& parcel) {
    stopped = parcel.read_bool();
    requested = parcel.read_bool();
}

void StopController::reset() {
    stopped = false;
    requested = false;
}
