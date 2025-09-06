#ifndef UTILSVECTOR_H
#define UTILSVECTOR_H

#include <cstdint>
#include <cstring>
#include <type_traits>

#include "utils/asserts.h"

/*
 * Vector that supports:
 * + push_back()
 * + pop_back()
 *
 *  0 1 2 3 4 5 6 7
 * |x|x|x|x|x| | | |
 *            ^       N = 8
 *            +- cursor = 5
 */
template <typename T, uint8_t N>
struct Vector {
    // static_assert(std::is_trivially_copyable_v<T>);

    static constexpr uint8_t Size = N;

    bool is_empty() const {
        return cursor == 0;
    }

    bool is_not_empty() const {
        return cursor > 0;
    }

    bool is_full() const {
        return cursor == N;
    }

    bool is_not_full() const {
        return cursor < N;
    }

    uint8_t size() const {
        return cursor;
    }

    void push_back(T element) {
        ASSERT(!is_full());
        data[cursor++] = element;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        ASSERT(!is_full());
        data[cursor++] = T {std::forward<Args>(args)...};
    }

    T pull_back() {
        ASSERT(is_not_empty());
        return data[--cursor];
    }

    void pop_back() {
        ASSERT(is_not_empty());
        --cursor;
    }

    void fill() {
        cursor = N;
    }

    T& back() {
        ASSERT(is_not_empty());
        return data[cursor - 1];
    }

    void clear() {
        cursor = 0;
    }

    const T& operator[](uint8_t index) const {
        ASSERT(index < N);
        ASSERT(index < size());
        return data[index];
    }

    T& operator[](uint8_t index) {
        ASSERT(index < N);
        ASSERT(index < size());
        return data[index];
    }

    T* begin() {
        return data;
    }

    T* end() {
        return data + cursor;
    }

    const T* begin() const {
        return data;
    }

    const T* end() const {
        return data + cursor;
    }

    T data[N] {};
    uint8_t cursor {};
};

#endif // UTILSVECTOR_H