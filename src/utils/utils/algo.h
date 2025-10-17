#ifndef UTILSALGO_H
#define UTILSALGO_H

#include <algorithm>

template <typename InType, typename OutType>
OutType clamp(const InType& in, const OutType& low, const OutType& high) {
    return static_cast<OutType>(std::clamp(in, static_cast<InType>(low), static_cast<InType>(high)));
}

template <typename T>
std::pair<T, T> divide(const T a, const T b) {
    T q = a / b;
    T r = a - q * b;
    return {q, r};
}

#endif // UTILSALGO_H