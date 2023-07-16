template <uint8_t n, typename T>
uint8_t get_byte(T value) {
    return value >> (8 * n);
}

template <uint8_t n, typename T>
void set_byte(T& dest, uint8_t value) {
    dest &= (T)(~(((T)(0xFF)) << (8 * n)));
    dest |= (value << (8 * n));
}

template <uint8_t n, typename T>
uint8_t get_nibble(T value) {
    return (value >> (4 * n)) & 0xF;
}

template <uint8_t n, typename T>
void set_nibble(T& dest, uint8_t value) {
    dest &= (T)(~(((T)(0xF)) << (4 * n)));
    dest |= ((value & 0xF) << (4 * n));
}

template <typename T>
bool get_bit(T value, uint8_t n) {
    return (value >> n) & 1;
}

template <uint8_t n, typename T>
bool get_bit(T value) {
    return (value >> n) & 1;
}

template <typename T>
T set_bit(T&& dest, uint8_t n, bool value) {
    dest &= (~((uint8_t)1 << n));
    dest |= ((value ? 1 : 0) << n);
    return dest;
}

template <uint8_t n, typename T>
T set_bit(T&& dest, bool value) {
    dest &= (~((uint8_t)1 << n));
    dest |= ((value ? 1 : 0) << n);
    return dest;
}

template <typename T>
T reset_bit(T&& dest, uint8_t n) {
    return set_bit(dest, n, false);
}

template <uint8_t n, typename T>
T reset_bit(T&& dest) {
    return set_bit<n>(dest, false);
}

template <uint8_t n, typename T>
T keep_bits(T value) {
    return value & bitmask_on<n>;
}

template <uint8_t n, typename T>
T reset_bits(T value) {
    return value & bitmask_off<n>;
}

template <uint8_t b, typename T1, typename T2>
bool sum_get_carry_bit(T1 v1, T2 v2) {
    uint64_t mask = bitmask_on<b + 1>;
    return ((((uint64_t)v1) & mask) + (((uint64_t)v2) & mask)) & bit<b + 1>;
    //    T1 result = v1 + v2;
    //    return (v1 ^ v2 ^ result) & bit<b + 1>;
    //    if (v2 > 0) {
    //        uint64_t vm1 = (((uint64_t) v1) & mask);
    //        uint64_t vm2 = (((uint64_t) v2) & mask);
    //        uint64_t result = vm1 + vm2;
    //        return get_bit<b + 1>(result);
    //    } else {
    //        v2 = v2 < 0 ? -v2 : v2;
    //        uint64_t vm1 = (((uint64_t) v1) & mask);
    //        uint64_t vm2 = (((uint64_t) v2) & mask);
    //        return vm2 > vm1;
    //    }
}

template <uint8_t b, typename T1, typename T2>
std::tuple<T1, bool> sum_carry(T1 v1, T2 v2) {
    T1 result = v1 + v2;
    return std::make_tuple(result, sum_get_carry_bit<b>(v1, v2));
}

template <uint8_t b1, uint8_t b2, typename T1, typename T2>
std::tuple<T1, bool, bool> sum_carry(T1 v1, T2 v2) {
    T1 result = v1 + v2;
    return std::make_tuple(result, sum_get_carry_bit<b1>(v1, v2), sum_get_carry_bit<b2>(v1, v2));
}
//
//
template <uint8_t b, typename T1, typename T2>
bool sub_get_borrow_bit(T1 v1, T2 v2) {
    uint64_t mask = bitmask_on<b + 1>;
    //    return ((((uint64_t) v1) & mask) - (((uint64_t) v2) & mask)) & bit<b + 1>;
    v2 = (v2 < 0) ? -v2 : v2;
    uint64_t vm1 = (((uint64_t)v1) & mask);
    uint64_t vm2 = (((uint64_t)v2) & mask);
    return vm2 > vm1;
}

template <uint8_t b, typename T1, typename T2>
std::tuple<T1, bool> sub_borrow(T1 v1, T2 v2) {
    T1 result = v1 - v2;
    return std::make_tuple(result, sub_get_borrow_bit<b>(v1, v2));
}

template <uint8_t b1, uint8_t b2, typename T1, typename T2>
std::tuple<T1, bool, bool> sub_borrow(T1 v1, T2 v2) {
    T1 result = v1 - v2;
    return std::make_tuple(result, sub_get_borrow_bit<b1>(v1, v2), sub_get_borrow_bit<b2>(v1, v2));
}

template <typename T>
std::string bin(T value) {
    std::stringstream ss;
    bin(value, ss);
    return ss.str();
}

template <typename T>
std::string bin(const T* data, size_t length) {
    std::stringstream ss;

    for (size_t i = 0; i < length; i++) {
        bin(data[i], ss);
        if (i < length - 1)
            ss << " ";
    }
    return ss.str();
}

template <typename T>
std::string hex(T value) {
    std::stringstream ss;
    hex(value, ss);
    return ss.str();
}

template <typename T>
std::string hex(const T* data, size_t length) {
    std::stringstream ss;

    for (size_t i = 0; i < length; i++) {
        hex(data[i], ss);
        if (i < length - 1)
            ss << " ";
    }
    return ss.str();
}
