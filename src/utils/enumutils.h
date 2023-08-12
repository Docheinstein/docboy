#ifndef ENUMUTILS_H
#define ENUMUTILS_H

#include <type_traits>

template <typename E, typename T>
constexpr inline typename std::enable_if<std::is_enum<E>::value && std::is_integral<T>::value, E>::type
to_enum(T value);

template <typename T, typename E>
constexpr inline typename std::enable_if<std::is_enum<E>::value && std::is_integral<T>::value, T>::type
from_enum(E value);

#include "enumutils.tpp"

#endif // ENUMUTILS_H