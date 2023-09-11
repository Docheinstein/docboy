#include <cassert>

template <template <typename...> class CONTAINER, typename T>
const T& pop(CONTAINER<T>& container) {
    assert(!container.empty());
    const auto& value = container.back();
    container.pop_back();
    return value;
}

template <template <typename...> class CONTAINER, typename T>
const T& pop_front(CONTAINER<T>& container) {
    assert(!container.empty());
    const auto& value = container.front();
    container.pop_front();
    return value;
}
