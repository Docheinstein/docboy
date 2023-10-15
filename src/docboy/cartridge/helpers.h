#ifndef CARTRIDGEHELPERS_H
#define CARTRIDGEHELPERS_H

#include "utils/bits.hpp"
#include "utils/math.h"

template <uint32_t m>
auto masked(uint32_t n) {
    return keep_bits<log_2<m>>(n);
}

#endif // CARTRIDGEHELPERS_H