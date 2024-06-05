#ifndef JOYPAD_H
#define JOYPAD_H

#include <cstdint>

class InterruptsIO;

class JoypadIO {
public:
    uint8_t read_p1() const;
    void write_p1(uint8_t value);

protected:
    uint8_t p1 {0b11000000};
    uint8_t keys {0b11111111};
};

class Joypad : public JoypadIO {
public:
    enum class Key : uint8_t {
        Down = 7,
        Up = 6,
        Left = 5,
        Right = 4,
        Start = 3,
        Select = 2,
        B = 1,
        A = 0,
    };

    enum class KeyState : uint8_t {
        Pressed = 0,
        Released = 1,
    };

    explicit Joypad(InterruptsIO& interrupts);

    void set_key_state(Key key, KeyState state);

private:
    InterruptsIO& interrupts;
};
#endif // JOYPAD_H