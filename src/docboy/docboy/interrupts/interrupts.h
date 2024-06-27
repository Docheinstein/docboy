#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/bits.h"
#include "utils/parcel.h"

class Interrupts {
public:
    enum class InterruptType {
        VBlank,
        Stat,
        Timer,
        Serial,
        Joypad,
    };

    void write_IF(uint8_t value) {
        IF = 0b11100000 | value;
    }

    template <InterruptType Type>
    void raise_Interrupt() {
        if constexpr (Type == InterruptType::VBlank) {
            set_bit<Specs::Bits::Interrupts::VBLANK>(IF);
        } else if constexpr (Type == InterruptType::Stat) {
            set_bit<Specs::Bits::Interrupts::STAT>(IF);
        } else if constexpr (Type == InterruptType::Timer) {
            set_bit<Specs::Bits::Interrupts::TIMER>(IF);
        } else if constexpr (Type == InterruptType::Serial) {
            set_bit<Specs::Bits::Interrupts::SERIAL>(IF);
        } else if constexpr (Type == InterruptType::Joypad) {
            set_bit<Specs::Bits::Interrupts::JOYPAD>(IF);
        }
    }

    void save_state(Parcel& parcel) const {
        parcel.write_uint8(IF);
        parcel.write_uint8(IE);
    }

    void load_state(Parcel& parcel) {
        IF = parcel.read_uint8();
        IE = parcel.read_uint8();
    }

    void reset() {
        IF = 0b11100001;
        IE = 0;
    }

    UInt8 IF {make_uint8(Specs::Registers::Interrupts::IF)};
    UInt8 IE {make_uint8(Specs::Registers::Interrupts::IE)};
};

#endif // INTERRUPTS_H