#ifndef UTILSFORMATTERS_H
#define UTILSFORMATTERS_H

#include <array>
#include <bitset>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

template <typename T>
void bin(T value, std::ostream& os) {
    os << std::bitset<8 * sizeof(T)>(value);
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
std::string bin(const std::vector<T>& vec) {
    return bin(vec.data(), vec.size() * sizeof(T));
}

template <typename T, uint8_t ByteWidth = 2>
void hex(T value, std::ostream& os) {
    os << std::uppercase << std::hex << std::setfill('0') << std::setw(ByteWidth * sizeof(T)) << +value;
}

template <typename T, uint8_t ByteWidth = 2>
std::string hex(T value) {
    std::stringstream ss;
    hex<T, ByteWidth>(value, ss);
    return ss.str();
}

template <typename T, bool Spacing = true, uint8_t ByteWidth = 2>
std::string hex(const T* data, size_t length) {
    std::stringstream ss;

    for (size_t i = 0; i < length; i++) {
        hex<T, ByteWidth>(data[i], ss);
        if constexpr (Spacing) {
            if (i < length - 1)
                ss << " ";
        }
    }
    return ss.str();
}

template <typename T, bool Spacing = true, uint8_t ByteWidth = 2>
std::string hex(const std::vector<T>& vec) {
    return hex<T, Spacing, ByteWidth>(vec.data(), vec.size() * sizeof(T));
}

template <typename T, std::size_t N, bool Spacing = true, uint8_t ByteWidth = 2>
std::string hex(const std::array<T, N>& arr) {
    return hex<T, Spacing, ByteWidth>(arr.data(), arr.size() * sizeof(T));
}

#endif // UTILSFORMATTERS_H