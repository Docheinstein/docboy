#ifndef UTILSFILLQUEUE_H
#define UTILSFILLQUEUE_H

#include <cstdint>
#include <cstring>
#include <type_traits>

#include "utils/asserts.h"

/*
 * Queue that supports:
 * + push_back()
 * + pop_front()
 * + fill()

 *  0 1 2 3 4 5 6 7
 * | | | |x|x|x|x|x|
 *        ^          N = 8
 *        +---- cursor = 3
 */
template <typename T, uint8_t N>
struct FillQueue {
    static_assert(std::is_trivially_copyable_v<T>);

    bool is_empty() const {
        return cursor == N;
    }

    bool is_not_empty() const {
        return cursor < N;
    }

    bool is_full() const {
        return cursor == 0;
    }

    bool is_not_full() const {
        return cursor > 0;
    }

    uint8_t size() const {
        return N - cursor;
    }

    void fill(const void* src) {
        memcpy(data, src, sizeof(data));
        cursor = 0;
    }

    void fill() {
        cursor = 0;
    }

    void push_back() {
        ASSERT(!is_full());
        --cursor;
    }

    void push_back(T element) {
        ASSERT(!is_full());
        data[--cursor] = element;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        ASSERT(!is_full());
        data[--cursor] = T {std::forward<Args>(args)...};
    }

    T pop_front() {
        ASSERT(is_not_empty());
        return data[cursor++];
    }

    void clear() {
        cursor = N;
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

    T data[N] {};
    uint8_t cursor {N};
};

#endif // UTILSFILLQUEUE_H