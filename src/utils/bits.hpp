#ifndef BITS_HPP
#define BITS_HPP

#include <type_traits>

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

inline uint64_t bit_(uint8_t n) {
    return uint64_t(1) << n;
};

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

// ---------- NIBBLES ----------

template <uint8_t n, typename T>
uint8_t get_nibble(T value) {
    static_assert(n < 2 * sizeof(T));
    return (value >> (4 * n)) & 0xF;
}

template <uint8_t n, typename T>
void set_nibble(T& dest, uint8_t value) {
    dest &= (T)(~(((T)(0xF)) << (4 * n)));
    dest |= ((value & 0xF) << (4 * n));
}

// ---------- BITS ----------

template <typename T>
std::decay_t<T> get_bit(T&& value, uint8_t n) {
    return value & bit_(n);
}

template <uint8_t n, typename T>
std::decay_t<T> get_bit(T&& value) {
    static_assert(n < 8 * sizeof(T));
    return value & bit<n>;
}

template <typename T>
bool test_bit(T&& value, uint8_t n) {
    return value & bit_(n);
}

template <uint8_t n, typename T>
bool test_bit(T&& value) {
    static_assert(n < 8 * sizeof(T));
    return value & bit<n>;
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

template <uint8_t n, typename T, bool y>
void set_bit(T&& dest) {
    if constexpr (y) {
        set_bit<n>(dest);
    } else {
        reset_bit<n>(dest);
    }
}

template <uint8_t n, typename T>
[[nodiscard]] auto keep_bits(T&& value) {
    static_assert(n < 8 * sizeof(T));
    return value & bitmask<n>;
}

template <uint8_t from, uint8_t to, typename T, std::enable_if_t<from >= to>* = nullptr>
[[nodiscard]] auto keep_bits_range(T&& value) {
    static_assert(from < 8 * sizeof(T));
    static_assert(to < 8 * sizeof(T));
    return value & bitmask_range<from, to>;
}

template <uint8_t n, typename T>
[[nodiscard]] auto discard_bits(T&& value) {
    static_assert(n < 8 * sizeof(T));
    return value & bitmask_off<n>;
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

#endif // BITS_HPP
