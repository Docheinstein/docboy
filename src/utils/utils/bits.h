#ifndef UTILSBITS_H
#define UTILSBITS_H

#include <cstdint>
#include <iostream>
#include <type_traits>
#include <utility>

#include "utils/bitrange.h"

// ---------- TYPES ----------

template <typename ResultType>
struct sum_carry_result {
    ResultType sum;
    bool carry;
};

template <typename ResultType>
struct sum_carry_result_2 {
    ResultType sum;
    bool carry1;
    bool carry2;
};

template <typename ResultType>
struct sub_borrow_result {
    ResultType sub;
    bool borrow;
};

template <typename ResultType>
struct sub_borrow_result_2 {
    ResultType sub;
    bool borrow1;
    bool borrow2;
};

// ---------- CONSTANTS ----------

template <uint8_t n>
constexpr uint64_t bit = uint64_t(1) << n;

template <uint8_t n>
constexpr uint64_t bitmask = bit<n> - 1;

template <uint8_t from, uint8_t to, std::enable_if_t<from >= to>* = nullptr>
constexpr uint64_t bitmask_range = bitmask<from - to + 1> << to;

template <uint8_t n>
constexpr uint64_t bitmask_off = uint64_t(-1) << n;

constexpr inline uint64_t bit_(uint8_t n) {
    return uint64_t(1) << n;
};

// ---------- MATHEMATICS ----------

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
constexpr uint64_t mod(uint64_t n) {
    if constexpr (is_power_of_2<m>) {
        return n & bitmask<log_2<m>>;
    }
    return n % m;
}

template <typename T>
double pow2(T n) {
    return (n < 0) ? (1.0 / (1 << -n)) : (1 << n);
}

// ---------- BYTES ----------

template <uint8_t n, typename T>
uint8_t get_byte(T&& value) {
    static_assert(n < sizeof(T));
    return value >> (8 * n);
}

template <uint8_t n, typename T>
void set_byte(T& dest, uint8_t value) {
    static_assert(n < sizeof(T));
    dest &= (std::decay_t<T>)(~(((std::decay_t<T>)(0xFF)) << (8 * n)));
    dest |= (value << (8 * n));
}

inline uint16_t concat(uint8_t msb, uint8_t lsb) {
    return ((uint16_t)msb) << 8 | lsb;
}

// ---------- BITS ----------

template <uint8_t n, uint8_t... ns>
constexpr auto bits() {
    if constexpr (sizeof...(ns) > 0) {
        return bit<n> | bits<ns...>();
    }

    return bit<n>;
}

template <typename T>
std::decay_t<T> get_bit(T&& value, uint8_t n) {
    return value & bit_(n);
}

template <uint8_t n, typename T>
std::decay_t<T> get_bit(T&& value) {
    static_assert(n < 8 * sizeof(T));
    return value & bit<n>;
}

template <uint8_t n, uint8_t... ns, typename T>
std::decay_t<T> get_bits(T&& value) {
    if constexpr (sizeof...(ns) > 0) {
        return get_bit<n>(std::forward<T>(value)) | get_bits<ns...>(std::forward<T>(value));
    }

    return get_bit<n>(std::forward<T>(value));
}

template <typename T>
constexpr bool test_bit(T&& value, uint8_t n) {
    return value & bit_(n);
}

template <uint8_t n, typename T>
bool test_bit(T&& value) {
    static_assert(n < 8 * sizeof(T));
    return value & bit<n>;
}

template <uint64_t value, uint8_t n>
constexpr bool test_bit() {
    static_assert(n < 64);
    return value & bit<n>;
}

template <uint8_t n, uint8_t... ns, typename T>
bool test_bits_any(T&& value) {
    if constexpr (sizeof...(ns) > 0) {
        return test_bit<n>(std::forward<T>(value)) || test_bits_any<ns...>(std::forward<T>(value));
    }

    return test_bit<n>(std::forward<T>(value));
}

template <uint8_t n, uint8_t... ns, typename T>
bool test_bits_all(T&& value) {
    if constexpr (sizeof...(ns) > 0) {
        return test_bit<n>(std::forward<T>(value)) && test_bits_any<ns...>(std::forward<T>(value));
    }

    return test_bit<n>(std::forward<T>(value));
}

template <typename T>
void set_bit(T&& dest, uint8_t n, bool value) {
    dest &= (~bit_(n));
    dest |= ((value ? 1 : 0) << n);
}

template <typename T>
void set_bit(T&& dest, uint8_t n) {
    dest |= (1 << n);
}

template <uint8_t n, typename T>
void set_bit(T&& dest, bool value) {
    static_assert(n < 8 * sizeof(T));
    dest &= ~bit<n>;
    dest |= ((value ? 1 : 0) << n);
}

template <uint8_t n, typename T>
void set_bit(T&& dest) {
    static_assert(n < 8 * sizeof(T));
    dest |= bit<n>;
}

template <uint8_t n, typename T>
void reset_bit(T&& dest) {
    static_assert(n < 8 * sizeof(T));
    dest &= ~bit<n>;
}

template <typename T>
void reset_bit(T&& dest, uint8_t n) {
    dest &= ~(1 << n);
}

template <uint8_t n, typename T>
void toggle_bit(T&& dest) {
    dest ^= bit<n>;
}

template <uint8_t n, typename T, bool y>
void set_bit(T&& dest) {
    if constexpr (y) {
        set_bit<n>(dest);
    } else {
        reset_bit<n>(dest);
    }
}

template <uint8_t n, typename T>
auto keep_bits(T&& value) {
    static_assert(n < 8 * sizeof(T));
    return value & bitmask<n>;
}

template <uint8_t n, typename T>
auto discard_bits(T&& value) {
    static_assert(n < 8 * sizeof(T));
    return value & bitmask_off<n>;
}

template <uint8_t msb, uint8_t lsb, typename T, std::enable_if_t<msb >= lsb>* = nullptr>
auto keep_bits_range(T&& value) {
    static_assert(msb < 8 * sizeof(T));
    static_assert(lsb < 8 * sizeof(T));
    return value & bitmask_range<msb, lsb>;
}

template <const BitRange& range, typename T>
auto keep_bits_range(T&& value) {
    return keep_bits_range<range.msb, range.lsb>(std::forward<T>(value));
};

template <uint8_t msb, uint8_t lsb, typename T>
auto get_bits_range(T&& value) {
    return keep_bits_range<msb, lsb>(std::forward<T>(value)) >> lsb;
};

template <const BitRange& range, typename T>
auto get_bits_range(T&& value) {
    return get_bits_range<range.msb, range.lsb>(std::forward<T>(value));
};

template <typename T>
T least_significant_bit(T n) {
    return n & -n;
}

template <uint32_t m, typename T>
T mask_by_pow2(T n) {
    static_assert(is_power_of_2<m>);
    return keep_bits<log_2<m>>(n);
}

// ---------- OPERATIONS ----------

template <uint8_t b, typename T1, typename T2>
bool sum_test_carry_bit(T1 v1, T2 v2) {
    static constexpr uint64_t mask = bitmask<b + 1>;
    return ((uint64_t(v1) & mask) + (uint64_t(v2) & mask)) & bit<b + 1>;
}

template <uint8_t b, typename T1>
bool inc_test_carry_bit(T1 v1) {
    static constexpr uint64_t mask = bitmask<b + 1>;
    return ((uint64_t(v1) & mask) + uint64_t(1)) & bit<b + 1>;
}

template <uint8_t b, typename T1, typename T2>
sum_carry_result<T1> sum_carry(T1 v1, T2 v2) {
    return {T1(v1 + v2), sum_test_carry_bit<b>(v1, v2)};
}

template <uint8_t b1, uint8_t b2, typename T1, typename T2>
sum_carry_result_2<T1> sum_carry(T1 v1, T2 v2) {
    return {T1(v1 + v2), sum_test_carry_bit<b1>(v1, v2), sum_test_carry_bit<b2>(v1, v2)};
}

template <uint8_t b, typename T1>
sum_carry_result<T1> inc_carry(T1 v1) {
    return {T1(v1 + 1), inc_test_carry_bit<b>(v1)};
}

template <uint8_t b, typename T1, typename T2>
bool sub_get_borrow_bit(T1 v1, T2 v2) {
    // TODO: can be optimized?
    uint64_t mask = bitmask<b + 1>;
    v2 = (v2 < 0) ? -v2 : v2;
    uint64_t vm1 = (((uint64_t)v1) & mask);
    uint64_t vm2 = (((uint64_t)v2) & mask);
    return vm2 > vm1;
}

template <uint8_t b, typename T1, typename T2>
sub_borrow_result<T1> sub_borrow(T1 v1, T2 v2) {
    return {T1(v1 - v2), sub_get_borrow_bit<b>(v1, v2)};
}

template <uint8_t b1, uint8_t b2, typename T1, typename T2>
sub_borrow_result_2<T1> sub_borrow(T1 v1, T2 v2) {
    return {T1(v1 - v2), sub_get_borrow_bit<b1>(v1, v2), sub_get_borrow_bit<b2>(v1, v2)};
}

#endif // UTILSBITS_H
