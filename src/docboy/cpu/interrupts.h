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

    BYTE(IF, Specs::Registers::Interrupts::IF, IF_BOOTROM_ELSE(0, 0x01));
    BYTE(IE, Specs::Registers::Interrupts::IE);
};

#endif // INTERRUPTS_H