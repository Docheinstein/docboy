#ifndef MATH_H
#define MATH_H

#include "bits.hpp"
#include <cstdint>

template <uint32_t n>
struct IsPowerOf2 {
    static constexpr uint32_t half = n >> 1;
    static constexpr bool value = (n & 1) == 0 && IsPowerOf2<half>::value;
};

template <>
struct IsPowerOf2<2> {
    static constexpr bool value = true;
};

template <uint32_t n>
constexpr bool is_power_of_2 = IsPowerOf2<n>::value;

template <uint32_t n>
struct Log2 {
    static_assert(is_power_of_2<n>);
    static constexpr uint32_t value = Log2<n / 2>::value + 1;
};

template <>
struct Log2<1> {
    static constexpr uint8_t value = 0;
};

template <uint32_t n>
constexpr uint32_t log_2 = Log2<n>::value;

template <uint8_t m>
static uint8_t pow2mod(uint8_t n) {
    static_assert(is_power_of_2<m>);
    return n & bitmask_on<log_2<m>>;
}

template <typename T>
static T pow2(T n) {
    return 1 << n;
}

#endif // MATH_H