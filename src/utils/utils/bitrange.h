#ifndef UTILSBITRANGE_H
#define UTILSBITRANGE_H

struct BitRange {
    constexpr BitRange(uint8_t msb_, uint8_t lsb_) :
        msb {msb_},
        lsb {lsb_} {
    }

    uint8_t msb {};
    uint8_t lsb {};
};

#endif // UTILSBITRANGE_H
