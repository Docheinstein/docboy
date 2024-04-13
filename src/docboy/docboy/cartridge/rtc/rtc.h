#ifndef RTC_H
#define RTC_H

#include "docboy/shared/specs.h"
#include "utils/bits.hpp"
#include <cstdint>

struct RtcRegisters {
    uint8_t seconds {};
    uint8_t minutes {};
    uint8_t hours {};
    struct {
        uint8_t low {};
        uint8_t high {};
    } days {};
};

struct Rtc : RtcRegisters {
    uint32_t cycles {};

    void tick() {
        if (test_bit<Specs::Bits::Rtc::DH::STOPPED>(days.high))
            return;

        if (++cycles == Specs::Frequencies::CPU) {
            cycles = 0;
            if (++seconds == 60) {
                seconds = 0;
                if (++minutes == 60) {
                    minutes = 0;
                    if (++hours == 24) {
                        hours = 0;
                        if (++days.low == 0) {
                            if (test_bit<Specs::Bits::Rtc::DH::DAY>(days.high))
                                set_bit<Specs::Bits::Rtc::DH::DAY_OVERFLOW>(days.high);
                            toggle_bit<Specs::Bits::Rtc::DH::DAY>(days.high);
                        }
                    }
                }
            }
        }
    }
};

#endif // RTC_H
