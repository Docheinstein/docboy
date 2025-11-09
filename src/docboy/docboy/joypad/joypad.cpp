#include "docboy/joypad/joypad.h"
#include "docboy/interrupts/interrupts.h"

#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
#include "docboy/cartridge/header.h"
#endif

#include "utils/parcel.h"

#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
Joypad::Joypad(Interrupts& interrupts, CartridgeHeader& header) :
#else
Joypad::Joypad(Interrupts& interrupts) :
#endif

    interrupts {interrupts}
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    ,
    header {header}
#endif
{
    reset();
}

void Joypad::set_key_state(Key key, KeyState state) {
    set_bit(keys, static_cast<uint8_t>(key), static_cast<uint8_t>(state));

    const bool dpad = test_bit<5>(static_cast<uint8_t>(key));

    const bool raise_interrupt =
        (state == KeyState::Pressed) && ((!p1.select_dpad && dpad) || (!p1.select_buttons && !dpad));

    if (raise_interrupt) {
        interrupts.raise_interrupt<Interrupts::InterruptType::Joypad>();
    }
}

uint8_t Joypad::read_p1() const {
    return 0b11000000 | p1.select_buttons << Specs::Bits::Joypad::P1::SELECT_ACTION_BUTTONS |
           p1.select_dpad << Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS | read_keys();
}

void Joypad::write_p1(uint8_t value) {
    p1.select_buttons = test_bit<Specs::Bits::Joypad::P1::SELECT_ACTION_BUTTONS>(value);
    p1.select_dpad = test_bit<Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS>(value);
}

void Joypad::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BOOL(parcel, p1.select_buttons);
    PARCEL_WRITE_BOOL(parcel, p1.select_dpad);
}

void Joypad::load_state(Parcel& parcel) {
    p1.select_buttons = parcel.read_bool();
    p1.select_dpad = parcel.read_bool();
}

void Joypad::reset() {
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    if (test_bit<Specs::Bits::Cartridge::CgbFlag::CGB_GAME>(header.cgb_flag())) {
        p1.select_buttons = false;
        p1.select_dpad = false;
    } else {
        p1.select_buttons = true;
        p1.select_dpad = true;
    }
#else
    p1.select_buttons = false;
    p1.select_dpad = false;
#endif
    keys = 0b11111111;
}

uint8_t Joypad::read_keys() const {
    return ((p1.select_dpad ? bitmask<4> : keep_bits<4>(keys >> 4)) &
            (p1.select_buttons ? bitmask<4> : keep_bits<4>(keys)));
}
