#ifndef STOPCONTROLLER_H
#define STOPCONTROLLER_H

#include <cstdint>

class Joypad;
class Timers;
class Ppu;
class Lcd;
class Parcel;

class StopController {
public:
    StopController(bool& stopped, Joypad& joypad, Timers& timers, Ppu& ppu, Lcd& lcd);

    void stop();

    void tick_t3() {
        if (requested) {
            enter_stop_mode();
        }
    }

    void stopped_tick();
    void stopped_tick_t3();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    void enter_stop_mode();

    bool& stopped;
    uint8_t requested {};

    Joypad& joypad;
    Timers& timers;
    Ppu& ppu;
    Lcd& lcd;

#ifndef ENABLE_CGB
    uint16_t ppu_shutdown_countdown {};
#endif
};

#endif // STOPCONTROLLER_H
