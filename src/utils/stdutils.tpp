#include <vector>

template<typename T>
T& pop(std::vector<T> &vector) {
    auto& value = vector.back();
    vector.pop_back();
    return value;
}