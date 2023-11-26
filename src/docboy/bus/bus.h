#ifndef BUS_H
#define BUS_H

#include "docboy/memory/fwd/bytefwd.h"

class IBus {};

template <typename Impl>
class Bus : public IBus {
public:
    static uint8_t read(const IBus* bus, uint16_t address);
    static void write(IBus* bus, uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

protected:
    struct MemoryAccess {
        using NonTrivialRead = uint8_t (Impl::*)(uint16_t) const;
        using NonTrivialWrite = void (Impl::*)(uint16_t, uint8_t);

        MemoryAccess() = default;
        MemoryAccess(byte* rw);
        MemoryAccess(byte* r, byte* w);
        MemoryAccess(NonTrivialRead r, byte* w);
        MemoryAccess(byte* r, NonTrivialWrite w);
        MemoryAccess(NonTrivialRead r, NonTrivialWrite w);

        struct Read {
            byte* trivial {};
            NonTrivialRead nonTrivial {};
        } read {};

        struct Write {
            byte* trivial {};
            NonTrivialWrite nonTrivial {};
        } write {};
    };

    MemoryAccess memoryAccessors[UINT16_MAX + 1] {};
};

#include "bus.hpp"

#endif // BUS_H