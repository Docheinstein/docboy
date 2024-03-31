#include "stopcontroller.h"
#include "docboy/joypad/joypad.h"
#include "docboy/lcd/lcd.h"
#include "docboy/timers/timers.h"
#include "utils/bits.hpp"
#include "utils/parcel.h"

StopController::StopController(bool& stopped, JoypadIO& joypad, TimersIO& timers, Lcd& lcd) :
    stopped(stopped),
    joypad(joypad),
    timers(timers),
    lcd(lcd) {
}

void StopController::stop() {
    check(!stopped);
    check(!requested);

    // Request STOP.
    requested = true;
}

void StopController::enterStopMode() {
    check(!stopped);
    check(requested);

    // Enter STOP mode.
    requested = false;
    stopped = true;

    // DIV is reset.
    timers.DIV = 0;

    // LCD is cleared.
    lcd.clear();
}

void StopController::eventuallyExitStopMode() {
    check(stopped);
    check(!requested);

    // Exit STOP mode if there's joypad input.
    stopped = keep_bits<4>(joypad.readP1()) == bitmask<4>;
}

void StopController::saveState(Parcel& parcel) const {
    parcel.writeBool(requested);
}
void StopController::loadState(Parcel& parcel) {
    requested = parcel.readUInt8();
}
