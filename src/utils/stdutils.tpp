#include <vector>

template<typename T>
const T& pop(std::vector<T> &vector) {
    assert(!vector.empty());
    const auto& value = vector.back();
    vector.pop_back();
    return value;
}
