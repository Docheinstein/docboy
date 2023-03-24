#include "joypad.h"
#include "utils/binutils.h"
#include "core/definitions.h"

/*
 * Bit 7 - Not used
 * Bit 6 - Not used
 * Bit 5 - P15 Select Action buttons    (0=Select)
 * Bit 4 - P14 Select Direction buttons (0=Select)
 * Bit 3 - P13 Input: Down  or Start    (0=Pressed) (Read Only)
 * Bit 2 - P12 Input: Up    or Select   (0=Pressed) (Read Only)
 * Bit 1 - P11 Input: Left  or B        (0=Pressed) (Read Only)
 * Bit 0 - P10 Input: Right or A        (0=Pressed) (Read Only)
 */

Joypad::Joypad(IInterruptsIO &interrupts) :
    interrupts(interrupts), P15(), P14(), keys() {

}

void Joypad::setKeyState(IJoypad::Key key, IJoypad::KeyState state) {
    keys[key] = state;
}

uint8_t Joypad::readP1() const {
    uint8_t P1 = 0xFF;
    bool directions = P14 == 0;
    set_bit<Bits::Joypad::P1::P15>(P1, P15);
    set_bit<Bits::Joypad::P1::P14>(P1, P14);
    set_bit<Bits::Joypad::P1::P13>(P1, !(directions ? keys[Down] : keys[Start]));
    set_bit<Bits::Joypad::P1::P12>(P1, !(directions ? keys[Up] : keys[Select]));
    set_bit<Bits::Joypad::P1::P11>(P1, !(directions ? keys[Left] : keys[B]));
    set_bit<Bits::Joypad::P1::P10>(P1, !(directions ? keys[Right] : keys[A]));
    return P1;
}

void Joypad::writeP1(uint8_t value) {
    P15 = get_bit<Bits::Joypad::P1::P15>(value);
    P14 = get_bit<Bits::Joypad::P1::P14>(value);
}
