#ifndef UTILSCASTS_H
#define UTILSCASTS_H

#include <cstdint>

inline int8_t to_signed(uint8_t value) {
    return value <= (INT8_MAX) ? static_cast<int8_t>(value) : static_cast<int8_t>(value - INT8_MIN) + INT8_MIN;
    //    return static_cast<int8_t>(value);
}

#endif // UTILSCASTS_H