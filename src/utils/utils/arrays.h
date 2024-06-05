#ifndef UTILSARRAYS_H
#define UTILSARRAYS_H

#include <cstddef>

template <typename T, size_t N>
constexpr size_t array_size(T (&)[N]) {
    return N;
}

#endif // UTILSARRAYS_H