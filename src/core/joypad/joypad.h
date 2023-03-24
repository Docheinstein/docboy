#ifndef JOYPAD_H
#define JOYPAD_H

#include "core/io/joypad.h"
#include "core/io/interrupts.h"


class IJoypad {
public:
    enum Key {
        Down,
        Up,
        Left,
        Right,
        Start,
        Select,
        B,
        A
    };
    enum KeyState {
        Pressed = 0,
        Released = 1
    };
    virtual ~IJoypad() = default;
    virtual void setKeyState(Key, KeyState) = 0;
};

class Joypad : public IJoypad, public IJoypadIO {
public:
    explicit Joypad(IInterruptsIO &interrupts);

    void setKeyState(Key, KeyState) override;

    [[nodiscard]] uint8_t readP1() const override;
    void writeP1(uint8_t value) override;

private:
    IInterruptsIO &interrupts;

    bool P15;
    bool P14;
    KeyState keys[8];
};


#endif // JOYPAD_H