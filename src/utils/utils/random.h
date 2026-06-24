#ifndef UTILSRANDOM_H
#define UTILSRANDOM_H

#include <cstdint>
#include <type_traits>

// https://rosettacode.org/wiki/Pseudo-random_numbers/Splitmix64
inline uint64_t splitmix64(uint64_t s) {
    uint64_t z = s + 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

template <uint32_t min = 0, uint32_t max = UINT32_MAX, std::enable_if_t<max >= min>* = nullptr>
class FastUniformRandomNumberGenerator {
public:
    explicit FastUniformRandomNumberGenerator(uint64_t seed = 0) :
        state {splitmix64(seed)} {
    }

    uint32_t next() {
        uint32_t s = advance_state() >> 32;
        return min + static_cast<uint32_t>((static_cast<uint64_t>(max - min + 1) * s) >> 32);
    }

private:
    uint64_t state {};

    uint64_t advance_state() {
        state = splitmix64(state);
        return state;
    }
};

#endif // UTILSRANDOM_H