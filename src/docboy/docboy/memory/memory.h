#ifndef MEMORY_H
#define MEMORY_H

#include <cstring>

#include "docboy/memory/cell.h"

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

    const UInt8& operator[](uint16_t index) const {
        return data[index];
    }
    UInt8& operator[](uint16_t index) {
        return data[index];
    }

    void save_state(Parcel& parcel) {
        parcel.write_bytes(data, Size * sizeof(UInt8));
    }
    void load_state(Parcel& parcel) {
        parcel.read_bytes(data, Size * sizeof(UInt8));
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

    UInt8 data[Size] {};
};

#endif // MEMORY_H