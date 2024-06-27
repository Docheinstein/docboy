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
    timers.div = 0;

    // LCD is cleared
    lcd.clear();
}

void StopController::eventually_exit_stop_mode() {
    ASSERT(stopped);
    ASSERT(!requested);

    // Exit STOP mode if there's joypad input
    stopped = keep_bits<4>(joypad.p1.read()) == bitmask<4>;
}

void StopController::save_state(Parcel& parcel) const {
    parcel.write_bool(requested);
}

void StopController::load_state(Parcel& parcel) {
    requested = parcel.read_uint8();
}

void StopController::reset() {
    requested = false;
}
