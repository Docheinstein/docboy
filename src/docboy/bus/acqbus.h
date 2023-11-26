#ifndef ACQBUS_H
#define ACQBUS_H

#include "bus.h"
#include "utils/parcel.h"

struct AcquirableBusDevice {
    using Type = uint8_t;
    static constexpr Type Dma = 1 << 0;
    static constexpr Type Ppu = 1 << 1;
};

template <typename Impl>
class AcquirableBus : public Bus<Impl> {
public:
    using Device = AcquirableBusDevice;

    template <Device::Type d>
    void acquire() {
        set_bit<d>(acquirers);
    }

    template <Device::Type d>
    void release() {
        reset_bit<d>(acquirers);
    }

    [[nodiscard]] bool isAcquired() const {
        return acquirers;
    }

    template <Device::Type d>
    [[nodiscard]] bool isAcquiredBy() const {
        return test_bit<d>(acquirers);
    }

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(acquirers);
    }

    void loadState(Parcel& parcel) {
        acquirers = parcel.readUInt8();
    }

private:
    uint8_t acquirers {};
};

template <typename Bus, AcquirableBusDevice::Type d>
class AcquirableBusProxy {
public:
    explicit AcquirableBusProxy(Bus& bus) :
        bus(bus) {
    }

    const byte& operator[](uint16_t index) const {
        return bus[index];
    }

    byte& operator[](uint16_t index) {
        return bus[index];
    }

    void acquire() {
        bus.template acquire<d>();
    }

    void release() {
        bus.template release<d>();
    }

    [[nodiscard]] bool isAcquired() const {
        return bus.template isAcquiredBy<d>();
    }

private:
    Bus& bus;
};

#endif // ACQBUS_H