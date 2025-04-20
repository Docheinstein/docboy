#ifndef SPEEDSWITCHCONTROLLER_H
#define SPEEDSWITCHCONTROLLER_H

#include <cstdint>

#include "docboy/common/macros.h"
#include "docboy/speedswitch/speedswitch.h"

class Interrupts;
class Timers;
class Parcel;

class SpeedSwitchController {
    DEBUGGABLE_CLASS()

public:
    SpeedSwitchController(SpeedSwitch& speed_switch, Interrupts& interrupts, Timers& timers, bool& halted);

    void tick();

    void switch_speed();

    bool is_double_speed_mode() const {
        return speed_switch.key1.current_speed;
    }

    bool is_switching_speed() const {
        return speed_switch_countdown > 0;
    }

    bool is_blocking_timers() const {
        return timers_block.blocked;
    }

    bool is_blocking_interrupts() const {
        return interrupts_block.blocked;
    }

    bool is_blocking_dma() const {
        return dma_block.blocked;
    }

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

public:
    SpeedSwitch& speed_switch;
    Interrupts& interrupts;
    Timers& timers;
    bool& halted;

    uint32_t speed_switch_countdown {};

    struct {
        bool blocked {};
        uint8_t countdown {};
    } timers_block {};

    struct {
        bool blocked {};
        uint8_t countdown {};
    } interrupts_block {};

    struct {
        bool blocked {};
        uint8_t countdown {};
    } dma_block {};
};

#endif // SPEEDSWITCHCONTROLLER_H
