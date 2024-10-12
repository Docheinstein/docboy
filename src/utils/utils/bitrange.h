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

inline uint8_t operator<<(uint8_t v, BitRange r) {
    return v << r.lsb;
}

#endif // UTILSBITRANGE_H
