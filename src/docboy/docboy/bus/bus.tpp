#include "docboy/memory/byte.h"

#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/parcel.h"

template<typename Impl>
template<Device::Type Dev>
void Bus<Impl>::read_request(uint16_t addr) {
    set_bit<R<Dev>>(requests);

    if constexpr (Dev == Device::Cpu) {
        if (test_bits_or<R<Device::Dma>, W<Device::Dma>>(requests)) {
            return;
        }
    }

    address = addr;
}

template<typename Impl>
template<Device::Type Dev>
uint8_t Bus<Impl>::flush_read_request() {
    ASSERT(test_bit<R<Dev>>(requests));
    reset_bit<R<Dev>>(requests);
    return read_bus(address);
}

template<typename Impl>
template<Device::Type Dev>
void Bus<Impl>::write_request(uint16_t addr) {
    set_bit<W<Dev>>(requests);

    if constexpr (Dev == Device::Cpu) {
        if (test_bits_or<R<Device::Dma>, W<Device::Dma>>(requests)) {
            return;
        }
    }

    address = addr;
}

template<typename Impl>
template<Device::Type Dev>
void Bus<Impl>::flush_write_request(uint8_t value) {
    ASSERT(test_bit<W<Dev>>(requests));
    reset_bit<W<Dev>>(requests);
    write_bus(address, value);
}

template <typename Impl>
void Bus<Impl>::load_state(Parcel& parcel) {
    requests = parcel.read_uint8();
    address = parcel.read_uint16();
}

template <typename Impl>
void Bus<Impl>::save_state(Parcel& parcel) const {
    parcel.write_uint8(requests);
    parcel.write_uint16(address);
}


template <typename Impl>
void Bus<Impl>::reset() {
    requests = 0;
    address = 0;
}

template <typename Impl>
uint8_t Bus<Impl>::read_bus(uint16_t addr) const {
    const typename MemoryAccess::Read& read_access = memory_accessors[addr].read;
    ASSERT(read_access.trivial || read_access.non_trivial);

    return read_access.trivial ? static_cast<uint8_t>(*read_access.trivial)
                              : (static_cast<const Impl*>(this)->*(read_access.non_trivial))(addr);
}

template <typename Impl>
void Bus<Impl>::write_bus(uint16_t addr, uint8_t value) {
    const typename MemoryAccess::Write& write_access = memory_accessors[addr].write;
    ASSERT(write_access.trivial || write_access.non_trivial);

    if (write_access.trivial) {
        *write_access.trivial = value;
    } else {
        (static_cast<Impl*>(this)->*(write_access.non_trivial))(addr, value);
    }
}

template<typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::read_request(uint16_t addr) {
    bus.template read_request<Dev>(addr);
}

template<typename Bus, Device::Type Dev>
uint8_t BusView<Bus, Dev>::flush_read_request() {
    return bus.template flush_read_request<Dev>();
}

template<typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::write_request(uint16_t addr) {
    bus.template write_request<Dev>(addr);
}

template<typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::flush_write_request(uint8_t value) {
    bus.template flush_write_request<Dev>(value);
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(byte* rw) {
    read.trivial = rw;
    write.trivial = rw;
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(const byte* r, byte* w) {
    read.trivial = r;
    write.trivial = w;
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(NonTrivialRead r, byte* w) {
    read.non_trivial = r;
    write.trivial = w;
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(const byte* r, NonTrivialWrite w) {
    read.trivial = r;
    write.non_trivial = w;
}

template <typename Impl>
Bus<Impl>::MemoryAccess::MemoryAccess(NonTrivialRead r, NonTrivialWrite w) {
    read.non_trivial = r;
    write.non_trivial = w;
}
