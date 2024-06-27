#ifndef BUS_H
#define BUS_H

#include <type_traits>

#include "docboy/bus/device.h"
#include "docboy/common/macros.h"
#include "docboy/memory/fwd/cellfwd.h"

class Parcel;

class IBus {};

template <typename Bus, Device::Type Dev>
class BusView;

class Bus : public IBus {
    DEBUGGABLE_CLASS()

public:
    template <typename Bus, Device::Type Dev>
    using View = BusView<Bus, Dev>;

    template <Device::Type Dev>
    void read_request(uint16_t addr);

    template <Device::Type Dev>
    uint8_t flush_read_request();

    template <Device::Type Dev>
    void write_request(uint16_t addr);

    template <Device::Type Dev>
    void flush_write_request(uint8_t value);

#ifdef ENABLE_DEBUGGER
    uint8_t read_bus(uint16_t address) const;
    void write_bus(uint16_t address, uint8_t value);
#endif

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    uint8_t requests {};
    uint16_t address {};

protected:
    struct NonTrivialReadFunctor {
        using Function = uint8_t (*)(void*, uint16_t);

        Function function {};
        void* target {};
    };

    struct NonTrivialWriteFunctor {
        using Function = void (*)(void*, uint16_t, uint8_t);

        Function function {};
        void* target {};
    };

    template <typename T, auto Read>
    struct NonTrivialRead : NonTrivialReadFunctor {
        explicit NonTrivialRead(T* t) {
            target = t;
            function = [](void* p, uint16_t addr) {
                if constexpr (std::is_invocable_v<decltype(Read), T&>) {
                    return (static_cast<T*>(p)->*Read)();
                } else if constexpr (std::is_invocable_v<decltype(Read), const T&>) {
                    return (static_cast<const T*>(p)->*Read)();
                } else if constexpr (std::is_invocable_v<decltype(Read), T&, uint16_t>) {
                    return (static_cast<T*>(p)->*Read)(addr);
                } else if constexpr (std::is_invocable_v<decltype(Read), const T&, uint16_t>) {
                    return (static_cast<const T*>(p)->*Read)(addr);
                } else {
                    static_assert(false);
                }
            };
        }
    };

    template <typename T, auto Write>
    struct NonTrivialWrite : NonTrivialWriteFunctor {
        explicit NonTrivialWrite(T* t) {
            target = t;
            function = [](void* p, uint16_t addr, uint8_t value) {
                if constexpr (std::is_invocable_v<decltype(Write), T&, uint8_t>) {
                    return (static_cast<T*>(p)->*Write)(value);
                } else if constexpr (std::is_invocable_v<decltype(Write), const T&, uint8_t>) {
                    return (static_cast<const T*>(p)->*Write)(value);
                } else if constexpr (std::is_invocable_v<decltype(Write), T&, uint16_t, uint8_t>) {
                    return (static_cast<T*>(p)->*Write)(addr, value);
                } else if constexpr (std::is_invocable_v<decltype(Write), const T&, uint16_t, uint8_t>) {
                    return (static_cast<const T*>(p)->*Write)(addr, value);
                } else {
                    static_assert(false);
                }
            };
        }
    };

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
    };

    template <typename T>
    struct CompositeMemoryAccess : MemoryAccess {
        explicit CompositeMemoryAccess(T* t) :
            MemoryAccess {NonTrivialRead<T, &T::read> {t}, NonTrivialWrite<T, &T::write> {t}} {
        }
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

template <typename Bus, Device::Type Dev>
class BusView {
public:
    explicit BusView(Bus& bus) :
        bus(bus) {
    }

    void read_request(uint16_t addr);
    uint8_t flush_read_request();
    void write_request(uint16_t addr);
    void flush_write_request(uint8_t value);

protected:
    Bus& bus;
};

#include "docboy/bus/bus.tpp"

#endif // BUS_H