#ifndef BUS_H
#define BUS_H

#include "device.h"
#include "docboy/debugger/macros.h"
#include "docboy/memory/fwd/bytefwd.h"

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
    void readRequest(uint16_t addr);

    template <Device::Type Dev>
    uint8_t flushReadRequest();

    template <Device::Type Dev>
    void writeRequest(uint16_t addr);

    template <Device::Type Dev>
    void flushWriteRequest(uint8_t value);

    uint8_t requests {};
    uint16_t address {};

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

    template <Device::Type Dev>
    static constexpr uint8_t R = 2 * Dev;

    template <Device::Type Dev>
    static constexpr uint8_t W = 2 * Dev + 1;

    IF_DEBUGGER(public:)
    [[nodiscard]] uint8_t readBus(uint16_t address) const;
    void writeBus(uint16_t address, uint8_t value);
    IF_DEBUGGER(protected:)

    MemoryAccess memoryAccessors[UINT16_MAX + 1] {};
};

template <typename Bus, Device::Type Dev>
class BusView {
public:
    BusView(Bus& bus) :
        bus(bus) {
    }

    void readRequest(uint16_t addr);
    uint8_t flushReadRequest();
    void writeRequest(uint16_t addr);
    void flushWriteRequest(uint8_t value);

protected:
    Bus& bus;
};

#include "bus.tpp"

#endif // BUS_H