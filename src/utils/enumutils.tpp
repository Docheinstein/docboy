template <typename E, typename T>
constexpr inline typename std::enable_if<std::is_enum<E>::value && std::is_integral<T>::value, E>::type
to_enum(T value) {
    return static_cast<E>(value);
}


template <typename T, typename E>
constexpr inline typename std::enable_if<std::is_enum<E>::value && std::is_integral<T>::value, T>::type
from_enum(E value) {
    return static_cast<T>(value);
}