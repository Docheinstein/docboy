#ifndef MATHEMATICS_H
#define MATHEMATICS_H

#include "bits.hpp"
#include <cstdint>

template <uint32_t n>
struct IsPowerOf2 {
    static constexpr uint32_t h = n >> 1;
    static constexpr bool value = (n & 1) == 0 && IsPowerOf2<h>::value;
};

template <>
struct IsPowerOf2<1> {
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

template <uint64_t m>
static uint64_t mod(uint64_t n) {
    if constexpr (is_power_of_2<m>) {
        return n & bitmask<log_2<m>>;
    }
    return n % m;
}

template <typename T>
static double pow2(T n) {
    return (n < 0) ? (1.0 / (1 << -n)) : (1 << n);
}

#endif // MATHEMATICS_H