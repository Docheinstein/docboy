#ifndef JOYPAD_H
#define JOYPAD_H

#include <cstdint>

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

class Interrupts;

class Joypad {
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

    explicit Joypad(Interrupts& interrupts);

    void set_key_state(Key key, KeyState state);

    struct P1 : Composite<P1, Specs::Registers::Joypad::P1> {
        uint8_t rd() const;
        void wr(uint8_t value);

        bool select_buttons {};
        bool select_dpad {};

        uint8_t keys {0b11111111};
    } p1 {};

private:
    Interrupts& interrupts;
};
#endif // JOYPAD_H