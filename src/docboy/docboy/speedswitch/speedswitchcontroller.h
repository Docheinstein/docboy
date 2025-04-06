#ifndef SPEEDSWITCHCONTROLLER_H
#define SPEEDSWITCHCONTROLLER_H

#include <cstdint>

#include "docboy/common/macros.h"
#include "docboy/speedswitch/speedswitch.h"
#include "docboy/timers/timers.h"

class SpeedSwitch;
class Parcel;

class SpeedSwitchController {
    DEBUGGABLE_CLASS()

public:
    SpeedSwitchController(SpeedSwitch& speed_switch, Timers& timers, bool& halted);

    void tick() {
        if (speed_switch_countdown) {
            tick_speed_switch();
        }
    }

    void switch_speed();

    bool is_double_speed_mode() const {
        return speed_switch.key1.current_speed;
    }

    bool is_switching_speed() const {
        return switching_speed;
    }

    bool is_blocking_timers() const {
        return timers_blocked;
    }

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    void tick_speed_switch();

    SpeedSwitch& speed_switch;
    Timers& timers;
    bool& halted;

    uint32_t speed_switch_countdown {};
    bool switching_speed {};

    uint8_t speed_switch_timers_block_countdown {};
    bool timers_blocked {};
};

#endif // SPEEDSWITCHCONTROLLER_H
