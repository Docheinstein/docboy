#ifndef HEXDUMP_LIBRARY_H
#define HEXDUMP_LIBRARY_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <iosfwd>

template<uint8_t n, typename T>
uint8_t get_byte(T value);

template<uint8_t n, typename T>
void set_byte(T &dest, uint8_t value);

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
std::string hexdump(const T *data, size_t length, int columns = 32);

template<typename T>
std::string hexdump(const std::vector<T> &vec);

#include "binutils.tpp"

#endif //HEXDUMP_LIBRARY_H