#ifndef RANDMEMORY_H
#define RANDMEMORY_H

#include "docboy/memory/memory.h"

#include "utils/random.h"

template <uint16_t Start, uint16_t End>
class RandomizedMemory : public Memory<Start, End> {
public:
    void reset() {
        static constexpr uint16_t size = Memory<Start, End>::Size;

        // NOTE: uint8_t is not supported in Windows: simulate it
        RandomNumberGenerator<uint32_t, 0, 255> rng /* use constant seed to be deterministic */;

        std::vector<uint8_t> data(size);
        for (uint16_t i = 0; i < size; i++) {
            data[i] = rng.next();
        }

        this->set_data(data.data(), size);
    }
};

#endif // RANDMEMORY_H