#include <algorithm>
#include <cassert>

template <template <typename...> class Container, typename T>
const T& pop(Container<T>& container) {
    assert(!container.empty());
    const auto& value = container.back();
    container.pop_back();
    return value;
}

template <template <typename...> class Container, typename T>
const T& pop_front(Container<T>& container) {
    assert(!container.empty());
    const auto& value = container.front();
    container.pop_front();
    return value;
}

template <template <typename...> class Container, typename T, typename Predicate>
void erase_if(Container<T>& container, Predicate predicate) {
    container.erase(std::remove_if(container.begin(), container.end(), predicate), container.end());
}

template <template <typename...> class Container, typename T>
bool contains(const Container<T>& container, const T& value) {
    return container.find(value) != container.end();
}