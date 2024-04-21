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
        if (requested)
            enterStopMode();
    }

    void handleStopMode() {
        eventuallyExitStopMode();
    }

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

    void reset();

private:
    void enterStopMode();
    void eventuallyExitStopMode();

    bool& stopped;
    bool requested {};

    JoypadIO& joypad;
    TimersIO& timers;
    Lcd& lcd;
};

#endif // STOPCONTROLLER_H
