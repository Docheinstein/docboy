#ifndef MEMORYTRAITS_H
#define MEMORYTRAITS_H

#include <type_traits>

#include "utils/traits.h"

template <auto F, typename = void>
struct IsReadMemberFunction : std::false_type {};

template <auto F>
struct IsReadMemberFunction<
    F, std::enable_if_t<std::is_same_v<uint8_t, decltype((std::declval<ClassOfMemberFunctionT<decltype(F)>>().*F)())>,
                        void>> : std::true_type {};

template <auto F>
static constexpr bool IsReadMemberFunctionV = IsReadMemberFunction<F>::value;

template <auto F, typename = void>
struct IsWriteMemberFunction : std::false_type {};

template <auto F>
struct IsWriteMemberFunction<
    F, std::enable_if_t<
           std::is_same_v<void, decltype((std::declval<ClassOfMemberFunctionT<decltype(F)>>().*F)(uint8_t {}))>, void>>
    : std::true_type {};

template <auto F>
static constexpr bool IsWriteMemberFunctionV = IsWriteMemberFunction<F>::value;

template <typename T, typename U = void>
struct HasReadMemberFunction : std::false_type {};

template <typename T>
struct HasReadMemberFunction<T, std::enable_if_t<std::is_same_v<uint8_t, decltype(std::declval<T>().read())>, void>>
    : std::true_type {};

template <typename T>
static constexpr bool HasReadMemberFunctionV = HasReadMemberFunction<T>::value;

template <typename T, typename U = void>
struct HasWriteMemberFunction : std::false_type {};

template <typename T>
struct HasWriteMemberFunction<
    T, std::enable_if_t<std::is_same_v<void, decltype(std::declval<T>().write(uint8_t {}))>, void>> : std::true_type {};

template <typename T>
static constexpr bool HasWriteMemberFunctionV = HasWriteMemberFunction<T>::value;

#endif // MEMORYTRAITS_H
