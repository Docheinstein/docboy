#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "byte.hpp"
#include "utils/parcel.h"
#include <cstring>

template <uint16_t Start, uint16_t End>
class Memory {
public:
#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
    Memory() {
        for (uint16_t i = 0; i < Size; i++) {
            data[i].setMemoryAddress(Start + i);
        }
    }
#else
    Memory() = default;
#endif

    Memory(const uint8_t* data, uint16_t length) :
        Memory() {
#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
        for (uint16_t i = 0; i < length; i++)
            this->data[i] = data[i];
#else
        memcpy(this->data, data, length);
#endif
    }

    const byte& operator[](uint16_t index) const {
        return data[index];
    }
    byte& operator[](uint16_t index) {
        return data[index];
    }

    void saveState(Parcel& parcel) {
        parcel.writeBytes(data, Size * sizeof(byte));
    }
    void loadState(Parcel& parcel) {
        parcel.readBytes(data, Size * sizeof(byte));
    }

private:
    static constexpr uint16_t Size = End - Start + 1;

    byte data[Size] {};
};

#endif // MEMORY_HPP