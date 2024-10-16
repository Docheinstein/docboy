#include "docboy/bus/bus.h"

#include "docboy/memory/cell.h"

#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/parcel.h"

inline uint8_t Bus::read_bus(uint16_t addr) const {
    const MemoryAccess::Read& read_access = memory_accessors[addr].read;
    ASSERT(read_access.trivial || read_access.non_trivial.function);

    return read_access.trivial ? static_cast<uint8_t>(*read_access.trivial)
                               : (read_access.non_trivial.function)(read_access.non_trivial.owner, addr);
}

inline void Bus::write_bus(uint16_t addr, uint8_t value) {
    const MemoryAccess::Write& write_access = memory_accessors[addr].write;
    ASSERT(write_access.trivial || write_access.non_trivial.function);

    if (write_access.trivial) {
        *write_access.trivial = value;
    } else {
        (write_access.non_trivial.function)(write_access.non_trivial.owner, addr, value);
    }
}

template <Device::Type Dev>
void Bus::read_request(uint16_t addr) {
    set_bit<R<Dev>>(requests);

    if constexpr (Dev == Device::Cpu) {
        if (test_bits_or<R<Device::Dma>, W<Device::Dma>>(requests)) {
            return;
        }
    }

    address = addr;
}

template <Device::Type Dev>
uint8_t Bus::flush_read_request() {
    ASSERT(test_bit<R<Dev>>(requests));
    reset_bit<R<Dev>>(requests);
    return read_bus(address);
}

template <Device::Type Dev>
void Bus::write_request(uint16_t addr) {
    set_bit<W<Dev>>(requests);

    if constexpr (Dev == Device::Cpu) {
        if (test_bits_or<R<Device::Dma>, W<Device::Dma>>(requests)) {
            return;
        }
    }

    address = addr;
}

template <Device::Type Dev>
void Bus::flush_write_request(uint8_t value) {
    ASSERT(test_bit<W<Dev>>(requests));
    reset_bit<W<Dev>>(requests);
    write_bus(address, value);
}

template <typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::read_request(uint16_t addr) {
    bus.template read_request<Dev>(addr);
}

template <typename Bus, Device::Type Dev>
uint8_t BusView<Bus, Dev>::flush_read_request() {
    return bus.template flush_read_request<Dev>();
}

template <typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::write_request(uint16_t addr) {
    bus.template write_request<Dev>(addr);
}

template <typename Bus, Device::Type Dev>
void BusView<Bus, Dev>::flush_write_request(uint8_t value) {
    bus.template flush_write_request<Dev>(value);
}

inline Bus::MemoryAccess::MemoryAccess(UInt8* rw) {
    read.trivial = rw;
    write.trivial = rw;
}

inline Bus::MemoryAccess::MemoryAccess(const UInt8* r, UInt8* w) {
    read.trivial = r;
    write.trivial = w;
}

inline Bus::MemoryAccess::MemoryAccess(Bus::NonTrivialReadFunctor r, UInt8* w) {
    read.non_trivial = r;
    write.trivial = w;
}

inline Bus::MemoryAccess::MemoryAccess(const UInt8* r, Bus::NonTrivialWriteFunctor w) {
    read.trivial = r;
    write.non_trivial = w;
}

inline Bus::MemoryAccess::MemoryAccess(Bus::NonTrivialReadFunctor r, Bus::NonTrivialWriteFunctor w) {
    read.non_trivial = r;
    write.non_trivial = w;
}

template <typename T>
std::enable_if_t<HasReadMemberFunctionV<T> && HasWriteMemberFunctionV<T>, Bus::MemoryAccess&> Bus::MemoryAccess::operator=(T* t) {
    *this = MemoryAccess {NonTrivialRead<&T::read> {t}, NonTrivialWrite<&T::write> {t}};
    return *this;
}

template <auto Read>
Bus::NonTrivialRead<Read>::NonTrivialRead(Bus::NonTrivialRead<Read>::OwnerType* t) {
    ASSERT(t);
    owner = t;
    function = [](void* p, uint16_t addr) {
        if constexpr (std::is_invocable_v<FunctionType, OwnerType&>) {
            return (static_cast<OwnerType*>(p)->*Read)();
        } else if constexpr (std::is_invocable_v<FunctionType, const OwnerType&>) {
            return (static_cast<const OwnerType*>(p)->*Read)();
        } else if constexpr (std::is_invocable_v<FunctionType, OwnerType&, uint16_t>) {
            return (static_cast<OwnerType*>(p)->*Read)(addr);
        } else if constexpr (std::is_invocable_v<FunctionType, const OwnerType&, uint16_t>) {
            return (static_cast<const OwnerType*>(p)->*Read)(addr);
        } else {
            static_assert(AlwaysFalseV<decltype(Read)>);
        }
    };
}

template <auto Write>
Bus::NonTrivialWrite<Write>::NonTrivialWrite(Bus::NonTrivialWrite<Write>::OwnerType* t) {
    ASSERT(t);
    owner = t;
    function = [](void* p, uint16_t addr, uint8_t value) {
        if constexpr (std::is_invocable_v<FunctionType, OwnerType&, uint8_t>) {
            return (static_cast<OwnerType*>(p)->*Write)(value);
        } else if constexpr (std::is_invocable_v<FunctionType, const OwnerType&, uint8_t>) {
            return (static_cast<const OwnerType>(p)->*Write)(value);
        } else if constexpr (std::is_invocable_v<FunctionType, OwnerType&, uint16_t, uint8_t>) {
            return (static_cast<OwnerType*>(p)->*Write)(addr, value);
        } else if constexpr (std::is_invocable_v<FunctionType, const OwnerType&, uint16_t, uint8_t>) {
            return (static_cast<const OwnerType*>(p)->*Write)(addr, value);
        } else {
            static_assert(AlwaysFalseV<decltype(Write)>);
        }
    };
}