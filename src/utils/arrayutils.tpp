template <typename T, size_t N>
constexpr size_t array_size(T(&)[N]) {
    return N;
}
