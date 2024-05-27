#ifndef FORMATTERS_HPP
#define FORMATTERS_HPP

#include <bitset>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>

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

template <typename T>
void hex(T value, std::ostream& os) {
    os << std::uppercase << std::hex << std::setfill('0') << std::setw(2 * sizeof(T)) << +value;
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

template <typename T>
std::string hex(const std::vector<T>& vec) {
    return hex(vec.data(), vec.size() * sizeof(T));
}

template <typename T, std::size_t N>
std::string hex(const std::array<T, N>& arr) {
    return hex(arr.data(), arr.size() * sizeof(T));
}

#endif // FORMATTERS_HPP