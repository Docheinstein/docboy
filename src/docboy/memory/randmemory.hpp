#ifndef RANDMEMORY_H
#define RANDMEMORY_H

#include "memory.hpp"
#include "utils/random.hpp"

template <uint16_t Start, uint16_t End>
class RandomizedMemory : public Memory<Start, End> {
public:
    RandomizedMemory() {
        static constexpr uint16_t size = Memory<Start, End>::Size;

        RandomNumberGenerator<uint8_t> rng /* use constant seed to be deterministic */;

        std::vector<uint8_t> data(size);
        for (uint16_t i = 0; i < size; i++) {
            data[i] = rng.next();
        }

        this->setData(data.data(), size);
    }
};

#endif // RANDMEMORY_H