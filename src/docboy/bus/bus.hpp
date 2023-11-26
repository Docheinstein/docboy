#include "docboy/memory/byte.hpp"
#include "utils/asserts.h"

template <typename Impl>
void Bus<Impl>::write(IBus* bus, uint16_t address, uint8_t value) {
    static_cast<Impl*>(bus)->write(address, value);
}

template <typename Impl>
uint8_t Bus<Impl>::read(const IBus* bus, uint16_t address) {
    return static_cast<const Impl*>(bus)->read(address);
}

template <typename Impl>
uint8_t Bus<Impl>::read(uint16_t address) const {
    const typename MemoryAccess::Read& readAccess = memoryAccessors[address].read;
    check(readAccess.trivial || readAccess.nonTrivial);

    return readAccess.trivial ? static_cast<uint8_t>(*readAccess.trivial)
                              : (static_cast<const Impl*>(this)->*(readAccess.nonTrivial))(address);
}

template <typename Impl>
void Bus<Impl>::write(uint16_t address, uint8_t value) {
    const typename MemoryAccess::Write& writeAccess = memoryAccessors[address].write;
    check(writeAccess.trivial || writeAccess.nonTrivial);

    if (writeAccess.trivial) {
        *writeAccess.trivial = value;
    } else {
        (static_cast<Impl*>(this)->*(writeAccess.nonTrivial))(address, value);
    }
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(byte* rw) {
    read.trivial = rw;
    write.trivial = rw;
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(byte* r, byte* w) {
    read.trivial = r;
    write.trivial = w;
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(NonTrivialRead r, byte* w) {
    read.nonTrivial = r;
    write.trivial = w;
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(byte* r, NonTrivialWrite w) {
    read.trivial = r;
    write.nonTrivial = w;
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(NonTrivialRead r, NonTrivialWrite w) {
    read.nonTrivial = r;
    write.nonTrivial = w;
}
