#include "docboy/memory/byte.hpp"
#include "utils/asserts.h"
#include "utils/bits.hpp"

template<typename Impl>
template<Device::Type Dev>
void Bus<Impl>::readRequest(uint16_t addr) {
    set_bit<R<Dev>>(requests);

    if constexpr (Dev == Device::Cpu) {
        if (test_bits_or<R<Device::Dma>, W<Device::Dma>>(requests))
            return;
    }

    address = addr;
}

template<typename Impl>
template<Device::Type Dev>
uint8_t Bus<Impl>::flushReadRequest() {
    check(test_bit<R<Dev>>(requests));
    reset_bit<R<Dev>>(requests);
    return readBus(address);
}

template<typename Impl>
template<Device::Type Dev>
void Bus<Impl>::writeRequest(uint16_t addr) {
    set_bit<W<Dev>>(requests);

    if constexpr (Dev == Device::Cpu) {
        if (test_bits_or<R<Device::Dma>, W<Device::Dma>>(requests))
            return;
    }

    address = addr;
}

template<typename Impl>
template<Device::Type Dev>
void Bus<Impl>::flushWriteRequest(uint8_t value) {
    check(test_bit<W<Dev>>(requests));
    reset_bit<W<Dev>>(requests);
    writeBus(address, value);
}

template <typename Impl>
void Bus<Impl>::loadState(Parcel& parcel) {
    requests = parcel.readUInt8();
    address = parcel.readUInt16();
}

template <typename Impl>
void Bus<Impl>::saveState(Parcel& parcel) const {
    parcel.writeUInt8(requests);
    parcel.writeUInt16(address);
}


template <typename Impl>
void Bus<Impl>::reset() {
    requests = 0;
    address = 0;
}

template <typename Impl>
uint8_t Bus<Impl>::readBus(uint16_t address) const {
    const typename MemoryAccess::Read& readAccess = memoryAccessors[address].read;
    check(readAccess.trivial || readAccess.nonTrivial);

    return readAccess.trivial ? static_cast<uint8_t>(*readAccess.trivial)
                              : (static_cast<const Impl*>(this)->*(readAccess.nonTrivial))(address);
}

template <typename Impl>
void Bus<Impl>::writeBus(uint16_t address, uint8_t value) {
    const typename MemoryAccess::Write& writeAccess = memoryAccessors[address].write;
    check(writeAccess.trivial || writeAccess.nonTrivial);

    if (writeAccess.trivial) {
        *writeAccess.trivial = value;
    } else {
        (static_cast<Impl*>(this)->*(writeAccess.nonTrivial))(address, value);
    }
}

template<typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::readRequest(uint16_t addr) {
    bus.template readRequest<Dev>(addr);
}

template<typename Bus, Device::Type Dev>
uint8_t BusView<Bus, Dev>::flushReadRequest() {
    return bus.template flushReadRequest<Dev>();
}

template<typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::writeRequest(uint16_t addr) {
    bus.template writeRequest<Dev>(addr);
}

template<typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::flushWriteRequest(uint8_t value) {
    bus.template flushWriteRequest<Dev>(value);
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
