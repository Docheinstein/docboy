#ifndef STOPCONTROLLER_H
#define STOPCONTROLLER_H

class JoypadIO;
class TimersIO;
class Lcd;
class Parcel;

class StopController {
public:
    StopController(bool& stopped, JoypadIO& joypad, TimersIO& timers, Lcd& lcd);

    void stop();

    void tick() {
        if (requested) {
            enter_stop_mode();
        }
    }

    void handle_stop_mode() {
        eventually_exit_stop_mode();
    }

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    void enter_stop_mode();
    void eventually_exit_stop_mode();

    bool& stopped;
    bool requested {};

    JoypadIO& joypad;
    TimersIO& timers;
    Lcd& lcd;
};

#endif // STOPCONTROLLER_H
