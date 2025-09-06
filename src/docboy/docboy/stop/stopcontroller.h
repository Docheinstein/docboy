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
    StopController(Joypad& joypad, Timers& timers, Ppu& ppu, Lcd& lcd);

    void stop();

    void tick() {
        if (requested) {
            enter_stop_mode();
        }
    }

    void stopped_tick();
    void stopped_tock();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    bool stopped;

private:
    void enter_stop_mode();

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
