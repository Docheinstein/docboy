#ifndef ARRAYUTILS_H
#define ARRAYUTILS_H

#include <cstddef>
#include <type_traits>

template <typename T, size_t N>
constexpr size_t array_size(T (&)[N]);

#include "arrayutils.tpp"

#endif // ARRAYUTILS_H