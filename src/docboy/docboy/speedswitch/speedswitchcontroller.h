#ifndef SPEEDSWITCHCONTROLLER_H
#define SPEEDSWITCHCONTROLLER_H

#include <cstdint>

#include "docboy/common/macros.h"
#include "docboy/speedswitch/speedswitch.h"
#include "docboy/timers/timers.h"

class SpeedSwitch;
class Parcel;
class Ppu;
class Lcd;

class SpeedSwitchController {
    DEBUGGABLE_CLASS()

public:
    SpeedSwitchController(SpeedSwitch& speed_switch, Timers& timers, Ppu& ppu, Lcd& lcd, bool& halted);

    void tick() {
        if (speed_switch_countdown) {
            if (--speed_switch_countdown == 0) {
                halted = false;
                timers.set_div(0);
            }
        }

        if (speed_switch_timers_block_countdown) {
            --speed_switch_timers_block_countdown;
        }
    }

    void switch_speed();

    bool is_double_speed_mode() const {
        return speed_switch.key1.current_speed;
    }

    // TODO: do it better (or use a boolean variable if really needed)
    bool is_switching_speed() const {
        return speed_switch_countdown > 0;
    }

    // TODO: do it better (or use a boolean variable if really needed)
    bool is_blocking_timers() const {
        return speed_switch_timers_block_countdown > 0;
    }

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    SpeedSwitch& speed_switch;
    Timers& timers;
    Ppu& ppu;
    Lcd& lcd;
    bool& halted;

    uint32_t speed_switch_countdown {};

    // TODO: why are timers blocked for some m cycles?
    uint32_t speed_switch_timers_block_countdown {};
};

#endif // SPEEDSWITCHCONTROLLER_H
