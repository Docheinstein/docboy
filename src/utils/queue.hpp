#ifndef QUEUE_HPP
#define QUEUE_HPP

#include "asserts.h"
#include <cstdint>
#include <type_traits>

/*
 * Queue that supports:
 * + push_back()
 * + pop_front()
 *
 *  0 1 2 3 4 5 6 7
 * | |x|x|x|x| | | |
 *    ^       ^       N = 8
 *    |       +---- end = 5
 *    +---------- start = 1
 *                count = 4
 */
template <typename T, uint8_t N>
struct Queue {
    static_assert(std::is_trivially_copyable_v<T>);

    [[nodiscard]] bool isEmpty() const {
        return count == 0;
    }
    [[nodiscard]] bool isNotEmpty() const {
        return count > 0;
    }

    [[nodiscard]] bool isFull() const {
        return count == N;
    }

    [[nodiscard]] bool isNotFull() const {
        return count < N;
    }

    [[nodiscard]] uint8_t size() const {
        return count;
    }

    void pushBack(T element) {
        check(!isFull());
        data[end] = element;
        end = (end + 1) % N;
        ++count;
    }

    T popFront() {
        check(isNotEmpty());
        T out = data[start];
        start = (start + 1) % N;
        --count;
        return out;
    }

    void clear() {
        count = 0;
        end = start;
    }

    const T& operator[](uint8_t index) const {
        check(index < N);
        check(index < size());
        return data[(start + index) % N];
    }

    T& operator[](uint8_t index) {
        check(index < N);
        check(index < size());
        return data[(start + index) % N];
    }

    T data[N] {};
    uint8_t start {};
    uint8_t end {};
    uint8_t count {};
};

#endif // QUEUE_HPP