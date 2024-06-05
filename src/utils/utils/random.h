#ifndef UTILSRANDOM_H
#define UTILSRANDOM_H

#include <random>

template <typename T, T min = 0, T max = std::numeric_limits<T>::max(),
          typename = std::enable_if_t<std::is_integral_v<T>>>
class RandomNumberGenerator {
public:
    explicit RandomNumberGenerator(uint32_t seed = std::mt19937::default_seed) :
        device {seed},
        engine {device()} {
    }

    T next() {
        return distribution(engine);
    }

private:
    std::mt19937 device;
    std::default_random_engine engine;
    std::uniform_int_distribution<T> distribution {min, max};
};

#endif // UTILSRANDOM_H