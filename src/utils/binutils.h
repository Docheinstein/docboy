#ifndef HEXDUMP_LIBRARY_H
#define HEXDUMP_LIBRARY_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <iosfwd>
#include <tuple>

template<uint8_t n, typename T>
uint8_t get_byte(T value);

template<uint8_t n, typename T>
void set_byte(T &dest, uint8_t value);

uint16_t concat_bytes(uint8_t msb, uint8_t lsb);

template<uint8_t n, typename T>
bool get_bit(T value);

template<uint8_t n, typename T>
void set_bit(T &dest, bool value);

template<uint8_t n>
uint64_t bitmask();


template<uint8_t b, typename T1, typename T2>
bool sum_get_carry_bit(T1 v1, T2 v2);

template<uint8_t b, typename T1, typename T2>
std::tuple<T1, bool> sum_carry(T1 v1, T2 v2);

template<uint8_t b1, uint8_t b2, typename T1, typename T2>
std::tuple<T1, bool, bool> sum_carry(T1 v1, T2 v2);


void bin(uint8_t value, std::ostream &os);
void bin(uint16_t value, std::ostream &os);

template<typename T>
std::string bin(T value);

template<typename T>
std::string bin(const T *data, size_t length);

template<typename T>
std::string bin(const std::vector<T> &vec) {
    return hex(vec.data(), vec.size() * sizeof(T));
}

void hex(uint8_t value, std::ostream &os);
void hex(uint16_t value, std::ostream &os);

template<typename T>
std::string hex(T value);

template<typename T>
std::string hex(const T *data, size_t length);

template<typename T>
std::string hex(const std::vector<T> &vec) {
    return hex(vec.data(), vec.size() * sizeof(T));
}

template<typename T>
std::string hexdump(const T *data, size_t length, bool addr = true, bool ascii = false, int columns = 32);

template<typename T>
std::string hexdump(const std::vector<T> &vec, bool addr = true, bool ascii = false, int columns = 32);

#include "binutils.tpp"

#endif //HEXDUMP_LIBRARY_H