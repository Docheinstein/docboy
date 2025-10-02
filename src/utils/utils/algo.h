#ifndef UTILSALGO_H
#define UTILSALGO_H

#include <algorithm>

template <typename InType, typename OutType>
OutType clamp(const InType& in, const OutType& low, const OutType& high) {
    return static_cast<OutType>(std::clamp(in, static_cast<InType>(low), static_cast<InType>(high)));
}

#endif // UTILSALGO_H