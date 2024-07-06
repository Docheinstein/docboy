#include "docboy/joypad/joypad.h"
#include "docboy/interrupts/interrupts.h"

Joypad::Joypad(Interrupts& interrupts) :
    interrupts {interrupts} {
}

void Joypad::set_key_state(Key key, KeyState state) {
    set_bit(keys, static_cast<uint8_t>(key), static_cast<uint8_t>(state));

    const bool raise_interrupt =
        (state == KeyState::Pressed) && (p1.select_dpad ? key <= Key::Start : key >= Key::Right);

    if (raise_interrupt) {
        interrupts.raise_Interrupt<Interrupts::InterruptType::Joypad>();
    }
}

uint8_t Joypad::read_p1() const {
    return 0b11000000 | p1.select_buttons << Specs::Bits::Joypad::P1::SELECT_ACTION_BUTTONS |
           p1.select_dpad << Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS |
           (p1.select_dpad ? keep_bits<4>(keys) : keep_bits<4>(keys >> 4));
}

void Joypad::write_p1(uint8_t value) {
    p1.select_buttons = test_bit<Specs::Bits::Joypad::P1::SELECT_ACTION_BUTTONS>(value);
    p1.select_dpad = test_bit<Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS>(value);
}
