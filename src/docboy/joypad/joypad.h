#ifndef JOYPAD_H
#define JOYPAD_H

#include <cstdint>

class InterruptsIO;

class JoypadIO {
public:
    [[nodiscard]] uint8_t readP1() const;
    void writeP1(uint8_t value);

protected:
    uint8_t P1 {0b11000000};
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

    void setKeyState(Key key, KeyState state);

private:
    InterruptsIO& interrupts;
};
#endif // JOYPAD_H