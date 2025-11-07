#include "docboy/bus/bus.h"

#include "docboy/memory/cell.h"

#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/parcel.h"

constexpr uint8_t BUS_DATA_DECAY_DELAY = 6;

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
        if (test_bits_any<R<Device::Dma>, W<Device::Dma>>(requests)) {
            return;
        }
    }

    address = addr;
}

template <Device::Type Dev>
uint8_t Bus::flush_read_request() {
    ASSERT(test_bit<R<Dev>>(requests));
    reset_bit<R<Dev>>(requests);

    data = read_bus(address);
    decay = BUS_DATA_DECAY_DELAY;

    return data;
}

template <Device::Type Dev>
uint8_t Bus::open_bus_read() {
    // TODO: verify whether this is what really happens.
    uint8_t bus_data = data;
    data = 0xFF;
    decay = 0;
    return bus_data;
}

template <Device::Type Dev>
void Bus::write_request(uint16_t addr) {
    set_bit<W<Dev>>(requests);

    if constexpr (Dev == Device::Cpu || Dev == Device::Idu) {
        if (test_bits_any<R<Device::Dma>, W<Device::Dma>>(requests)) {
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

    data = value;
    decay = BUS_DATA_DECAY_DELAY;
}

inline void Bus::tick() {
    // The bus seems to retain the value that was read/written value for some T-cycles.
    // (e.g. HDMA reads the byte that was previously on the external bus when the HDMA's source address is invalid).
    // TODO: verify the precise decay timing.
    if (decay > 0) {
        if (--decay == 0) {
            data = 0xFF;
        }
    }
}

template <typename BusType, Device::Type Dev>
void BusView<BusType, Dev>::read_request(uint16_t addr) {
    bus.template read_request<Dev>(addr);
}

template <typename BusType, Device::Type Dev>
uint8_t BusView<BusType, Dev>::flush_read_request() {
    return bus.template flush_read_request<Dev>();
}

template <typename BusType, Device::Type Dev>
uint8_t BusView<BusType, Dev>::open_bus_read() {
    return bus.template open_bus_read<Dev>();
}

template <typename BusType, Device::Type Dev>
void BusView<BusType, Dev>::write_request(uint16_t addr) {
    bus.template write_request<Dev>(addr);
}

template <typename BusType, Device::Type Dev>
void BusView<BusType, Dev>::flush_write_request(uint8_t value) {
    bus.template flush_write_request<Dev>(value);
}

constexpr Bus::MemoryAccess::MemoryAccess(UInt8* rw) {
    read.trivial = rw;
    write.trivial = rw;
}

constexpr Bus::MemoryAccess::MemoryAccess(const UInt8* r, UInt8* w) {
    read.trivial = r;
    write.trivial = w;
}

constexpr Bus::MemoryAccess::MemoryAccess(Bus::NonTrivialReadFunctor r, UInt8* w) {
    read.non_trivial = r;
    write.trivial = w;
}

constexpr Bus::MemoryAccess::MemoryAccess(const UInt8* r, Bus::NonTrivialWriteFunctor w) {
    read.trivial = r;
    write.non_trivial = w;
}

constexpr Bus::MemoryAccess::MemoryAccess(Bus::NonTrivialReadFunctor r, Bus::NonTrivialWriteFunctor w) {
    read.non_trivial = r;
    write.non_trivial = w;
}

template <typename T>
std::enable_if_t<HasReadMemberFunctionV<T> && HasWriteMemberFunctionV<T>, Bus::MemoryAccess&>
Bus::MemoryAccess::operator=(T* t) {
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