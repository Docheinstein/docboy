#ifndef BUS_H
#define BUS_H

#include "docboy/bus/device.h"
#include "docboy/common/macros.h"
#include "docboy/memory/fwd/bytefwd.h"

class Parcel;

class IBus {};

template <typename Bus, Device::Type Dev>
class BusView;

template <typename Impl>
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
    struct MemoryAccess {
        using NonTrivialRead = uint8_t (Impl::*)(uint16_t) const;
        using NonTrivialWrite = void (Impl::*)(uint16_t, uint8_t);

        MemoryAccess() = default;
        MemoryAccess(byte* rw);
        MemoryAccess(const byte* r, byte* w);
        MemoryAccess(NonTrivialRead r, byte* w);
        MemoryAccess(const byte* r, NonTrivialWrite w);
        MemoryAccess(NonTrivialRead r, NonTrivialWrite w);

        struct Read {
            const byte* trivial {};
            NonTrivialRead non_trivial {};
        } read {};

        struct Write {
            byte* trivial {};
            NonTrivialWrite non_trivial {};
        } write {};
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