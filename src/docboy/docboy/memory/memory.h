#ifndef MEMORY_H
#define MEMORY_H

#include <cstring>

#include "docboy/memory/byte.h"

#include "utils/asserts.h"
#include "utils/parcel.h"

template <uint16_t Start, uint16_t End>
class Memory {
public:
    static constexpr uint16_t Size = End - Start + 1;

    Memory() {
#ifdef ENABLE_DEBUGGER
        for (uint16_t i = 0; i < Size; i++) {
            data[i].address = Start + i;
        }
#endif
    }

    Memory(const uint8_t* data_, uint16_t length) :
        Memory {} {
        set_data(data_, length);
    }

    const byte& operator[](uint16_t index) const {
        return data[index];
    }
    byte& operator[](uint16_t index) {
        return data[index];
    }

    void save_state(Parcel& parcel) {
        parcel.write_bytes(data, Size * sizeof(byte));
    }
    void load_state(Parcel& parcel) {
        parcel.read_bytes(data, Size * sizeof(byte));
    }

protected:
    void set_data(const uint8_t* data_, uint16_t length) {
        ASSERT(length <= Size);

#ifdef ENABLE_DEBUGGER
        for (uint16_t i = 0; i < length; i++) {
            data[i] = data_[i];
        }
#else
        memcpy(this->data, data_, length);
#endif
    }

    byte data[Size] {};
};

#endif // MEMORY_H