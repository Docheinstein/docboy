#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "docboy/common/macros.h"
#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/bits.h"
#include "utils/parcel.h"

class Interrupts {
public:
    DEBUGGABLE_CLASS()

    enum class InterruptType {
        VBlank,
        Stat,
        Timer,
        Serial,
        Joypad,
    };

    void tick_t3() {
        // TODO: when is this reset in CGB double speed mode?
        block_interrupts = false;
    }

    void write_if(uint8_t value) {
        IF = 0b11100000 | value;

        // Writing to IF blocks IF for the rest of this M-Cycle.
        block_interrupts = true;
    }

    template <InterruptType Type>
    void raise_interrupt() {
        if (block_interrupts) {
            // IF is blocked the same M-Cycle it is written to.
            return;
        }

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
        PARCEL_WRITE_UINT8(parcel, IF);
        PARCEL_WRITE_UINT8(parcel, IE);
        PARCEL_WRITE_BOOL(parcel, block_interrupts);
    }

    void load_state(Parcel& parcel) {
        IF = parcel.read_uint8();
        IE = parcel.read_uint8();
        block_interrupts = parcel.read_bool();
    }

    void reset() {
        IF = 0b11100001;
        IE = 0;
        block_interrupts = false;
    }

    UInt8 IF {make_uint8(Specs::Registers::Interrupts::IF)};
    UInt8 IE {make_uint8(Specs::Registers::Interrupts::IE)};

private:
    bool block_interrupts {};
};

#endif // INTERRUPTS_H