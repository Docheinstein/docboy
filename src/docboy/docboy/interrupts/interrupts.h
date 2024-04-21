#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "docboy/bootrom/macros.h"
#include "docboy/memory/byte.hpp"
#include "docboy/shared/specs.h"
#include "utils/bits.hpp"
#include "utils/parcel.h"

class InterruptsIO {
public:
    enum class InterruptType {
        VBlank,
        Stat,
        Timer,
        Serial,
        Joypad,
    };

    void writeIF(uint8_t value) {
        IF = 0b11100000 | value;
    }

    template <InterruptType Type>
    void raiseInterrupt() {
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

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(IF);
        parcel.writeUInt8(IE);
    }

    void loadState(Parcel& parcel) {
        IF = parcel.readUInt8();
        IE = parcel.readUInt8();
    }

    void reset() {
        IF = 0b11100001;
        IE = 0;
    }

    byte IF {make_byte(Specs::Registers::Interrupts::IF)};
    byte IE {make_byte(Specs::Registers::Interrupts::IE)};
};

#endif // INTERRUPTS_H