#ifndef JOYPAD_H
#define JOYPAD_H

#include <cstdint>

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/bits.h"

class Interrupts;
class Parcel;
struct CartridgeHeader;

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

#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    Joypad(Interrupts& interrupts, CartridgeHeader& header);
#else
    explicit Joypad(Interrupts& interrupts);
#endif

    uint8_t read_p1() const;
    void write_p1(uint8_t value);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    void set_key_state(Key key, KeyState state);

    uint8_t read_keys() const;

    struct P1 : Composite<Specs::Registers::Joypad::P1> {
        Bool select_buttons {make_bool()};
        Bool select_dpad {make_bool()};
    } p1 {};

private:
    Interrupts& interrupts;
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    CartridgeHeader& header;
#endif

    uint8_t keys {0b11111111};
};
#endif // JOYPAD_H