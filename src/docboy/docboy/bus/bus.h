#ifndef BUS_H
#define BUS_H

#include <type_traits>

#include "docboy/bus/device.h"
#include "docboy/common/macros.h"
#include "docboy/memory/fwd/cellfwd.h"
#include "docboy/memory/traits.h"

#include "utils/asserts.h"

class Parcel;

class IBus {};

template <typename BusType, Device::Type Dev>
class BusView;

class Bus : public IBus {
    DEBUGGABLE_CLASS()

public:
    Bus();

    template <typename BusType, Device::Type Dev>
    using View = BusView<BusType, Dev>;

    template <Device::Type Dev>
    void read_request(uint16_t addr);

    template <Device::Type Dev>
    uint8_t flush_read_request();

    template <Device::Type Dev>
    uint8_t open_bus_read();

    template <Device::Type Dev>
    void write_request(uint16_t addr);

    template <Device::Type Dev>
    void flush_write_request(uint8_t value);

#ifdef ENABLE_DEBUGGER
    uint8_t read_bus(uint16_t address) const;
    void write_bus(uint16_t address, uint8_t value);
#endif

    void tick();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    uint8_t requests {};
    uint16_t address {};
    uint8_t data {};
    uint8_t decay {};

protected:
    template <typename Function>
    struct Functor {
        Function function;
        void* owner {};
    };

    using NonTrivialReadFunctor = Functor<uint8_t (*)(void*, uint16_t)>;
    using NonTrivialWriteFunctor = Functor<void (*)(void*, uint16_t, uint8_t)>;

    template <auto Read>
    struct NonTrivialRead : NonTrivialReadFunctor {
        using FunctionType = decltype(Read);
        using OwnerType = ClassOfMemberFunctionT<FunctionType>;

        explicit NonTrivialRead(OwnerType* t);
    };

    template <auto Write>
    struct NonTrivialWrite : NonTrivialWriteFunctor {
        using FunctionType = decltype(Write);
        using OwnerType = ClassOfMemberFunctionT<FunctionType>;

        explicit NonTrivialWrite(OwnerType* t);
    };

    template <auto F>
    using NonTrivial =
        std::conditional_t<IsReadMemberFunctionV<F>, NonTrivialRead<F>,
                           std::conditional_t<IsWriteMemberFunctionV<F>, NonTrivialWrite<F>, std::nullptr_t>>;

    struct MemoryAccess {
        struct Read {
            const UInt8* trivial {};
            NonTrivialReadFunctor non_trivial {};
        } read {};

        struct Write {
            UInt8* trivial {};
            NonTrivialWriteFunctor non_trivial {};
        } write {};

        MemoryAccess() = default;
        MemoryAccess(UInt8* rw);
        MemoryAccess(const UInt8* r, UInt8* w);
        MemoryAccess(NonTrivialReadFunctor r, UInt8* w);
        MemoryAccess(const UInt8* r, NonTrivialWriteFunctor w);
        MemoryAccess(NonTrivialReadFunctor r, NonTrivialWriteFunctor w);

        template <typename T>
        std::enable_if_t<HasReadMemberFunctionV<T> && HasWriteMemberFunctionV<T>, MemoryAccess&> operator=(T* t);
    };

    template <Device::Type Dev>
    static constexpr uint8_t R = 2 * Dev;

    template <Device::Type Dev>
    static constexpr uint8_t W = 2 * Dev + 1;

#ifndef ENABLE_DEBUGGER
    uint8_t read_bus(uint16_t address) const;
    void write_bus(uint16_t address, uint8_t value);
#endif

    MemoryAccess memory_accessors[UINT16_MAX + 1] {};
};

template <typename BusType, Device::Type Dev>
class BusView {
public:
    /* implicit */ BusView(BusType& bus) :
        bus(bus) {
    }

    void read_request(uint16_t addr);
    uint8_t flush_read_request();
    uint8_t open_bus_read();

    void write_request(uint16_t addr);
    void flush_write_request(uint8_t value);

protected:
    BusType& bus;
};

#include "docboy/bus/bus.tpp"

#endif // BUS_H