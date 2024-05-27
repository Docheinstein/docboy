#ifndef VECTOR_HPP
#define VECTOR_HPP

#include "asserts.h"
#include "type_traits"
#include <cstdint>
#include <cstring>

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

    [[nodiscard]] bool isEmpty() const {
        return cursor == 0;
    }
    [[nodiscard]] bool isNotEmpty() const {
        return cursor > 0;
    }

    [[nodiscard]] bool isFull() const {
        return cursor == N;
    }

    [[nodiscard]] bool isNotFull() const {
        return cursor < N;
    }

    [[nodiscard]] uint8_t size() const {
        return cursor;
    }

    void pushBack(T element) {
        check(!isFull());
        data[cursor++] = element;
    }

    template <typename... Args>
    void emplaceBack(Args&&... args) {
        check(!isFull());
        data[cursor++] = {std::forward<Args>(args)...};
    }

    T pullBack() {
        check(isNotEmpty());
        return data[--cursor];
    }

    void popBack() {
        check(isNotEmpty());
        --cursor;
    }

    T& back() {
        check(isNotEmpty());
        return data[cursor - 1];
    }

    void clear() {
        cursor = 0;
    }

    const T& operator[](uint8_t index) const {
        check(index < N);
        check(index < size());
        return data[index];
    }

    T& operator[](uint8_t index) {
        check(index < N);
        check(index < size());
        return data[index];
    }

    T data[N] {};
    uint8_t cursor {};
};

#endif // VECTOR_HPP