#ifndef UTILSTRAITS_H
#define UTILSTRAITS_H

#include <cstdint>

template <typename T>
struct ClassOfMemberFunction;

template <typename R, typename T>
struct ClassOfMemberFunction<R T::*> {
    using Type = T;
};

template <>
struct ClassOfMemberFunction<std::nullptr_t> {
    using Type = void;
};

template <typename T>
using ClassOfMemberFunctionT = typename ClassOfMemberFunction<T>::Type;

template <typename T>
using AlwaysFalse = std::false_type;

template <typename T>
inline constexpr auto AlwaysFalseV = AlwaysFalse<T>::value;

#endif // UTILSTRAITS_H
