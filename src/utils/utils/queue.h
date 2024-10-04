#ifndef UTILSQUEUE_H
#define UTILSQUEUE_H

#include <cstdint>
#include <type_traits>

#include "utils/asserts.h"

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

    bool is_empty() const {
        return count == 0;
    }

    bool is_not_empty() const {
        return count > 0;
    }

    bool is_full() const {
        return count == N;
    }

    bool is_not_full() const {
        return count < N;
    }

    uint8_t size() const {
        return count;
    }

    void push_back(T element) {
        ASSERT(!is_full());
        data[end] = element;
        end = (end + 1) % N;
        ++count;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        ASSERT(!is_full());
        data[end] = T {std::forward<Args>(args)...};
        end = (end + 1) % N;
        ++count;
    }

    T pop_front() {
        ASSERT(is_not_empty());
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
        ASSERT(index < N);
        ASSERT(index < size());
        return data[(start + index) % N];
    }

    T& operator[](uint8_t index) {
        ASSERT(index < N);
        ASSERT(index < size());
        return data[(start + index) % N];
    }

    T data[N] {};
    uint8_t start {};
    uint8_t end {};
    uint8_t count {};
};

#endif // UTILSQUEUE_H