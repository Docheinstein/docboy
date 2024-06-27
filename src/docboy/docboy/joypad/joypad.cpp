#include "docboy/joypad/joypad.h"
#include "docboy/interrupts/interrupts.h"

Joypad::Joypad(Interrupts& interrupts) :
    interrupts {interrupts} {
}

void Joypad::set_key_state(Key key, KeyState state) {
    set_bit(p1.keys, static_cast<uint8_t>(key), static_cast<uint8_t>(state));

    const bool raise_interrupt =
        (state == KeyState::Pressed) && (p1.select_dpad ? key <= Key::Start : key >= Key::Right);

    if (raise_interrupt) {
        interrupts.raise_Interrupt<Interrupts::InterruptType::Joypad>();
    }
}

uint8_t Joypad::P1::rd() const {
    return 0b11000000 | select_buttons << Specs::Bits::Joypad::P1::SELECT_ACTION_BUTTONS |
           select_dpad << Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS |
           (select_dpad ? keep_bits<4>(keys) : keep_bits<4>(keys >> 4));
}

void Joypad::P1::wr(uint8_t value) {
    select_buttons = test_bit<Specs::Bits::Joypad::P1::SELECT_ACTION_BUTTONS>(value);
    select_dpad = test_bit<Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS>(value);
}
