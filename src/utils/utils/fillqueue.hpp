#ifndef FILLQUEUE_HPP
#define FILLQUEUE_HPP

#include "asserts.h"
#include "type_traits"
#include <cstdint>
#include <cstring>

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

    [[nodiscard]] bool isEmpty() const {
        return cursor == N;
    }
    [[nodiscard]] bool isNotEmpty() const {
        return cursor < N;
    }

    [[nodiscard]] bool isFull() const {
        return cursor == 0;
    }

    [[nodiscard]] bool isNotFull() const {
        return cursor > 0;
    }

    [[nodiscard]] uint8_t size() const {
        return N - cursor;
    }

    void fill(const void* src) {
        memcpy(data, src, sizeof(data));
        cursor = 0;
    }

    void pushBack(T element) {
        check(!isFull());
        data[--cursor] = element;
    }

    T popFront() {
        check(isNotEmpty());
        return data[cursor++];
    }

    void clear() {
        cursor = N;
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
    uint8_t cursor {N};
};

#endif // FILLQUEUE_HPP