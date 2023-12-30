#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "byte.hpp"
#include "utils/asserts.h"
#include "utils/parcel.h"
#include <cstring>

template <uint16_t Start, uint16_t End>
class Memory {
public:
    static constexpr uint16_t Size = End - Start + 1;

#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
    Memory() {
        for (uint16_t i = 0; i < Size; i++) {
            data[i].setMemoryAddress(Start + i);
        }
    }
#else
    Memory() = default;
#endif

    Memory(const uint8_t* data_, uint16_t length) :
        Memory() {
        setData(data_, length);
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

protected:
    void setData(const uint8_t* data_, uint16_t length) {
        check(length <= Size);

#ifdef ENABLE_DEBUGGER_MEMORY_SNIFFER
        for (uint16_t i = 0; i < length; i++)
            data[i] = data_[i];
#else
        memcpy(this->data, data_, length);
#endif
    }

private:
    byte data[Size] {};
};

#endif // MEMORY_HPP