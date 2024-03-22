#include "utils/bits.hpp"
#include "utils/parcel.h"

template<typename Impl>
template<Device::Type Dev>
uint8_t VideoBus<Impl>::flushReadRequest() {
    check(test_bit<Bus<Impl>::template R<Dev>>(this->requests));

    if constexpr (Dev == Device::Cpu) {
        if (isAcquired()) {
            // CPU reads FF while bus is acquired either by PPU or DMA.
            reset_bit<Bus<Impl>::template R<Dev>>(this->requests);
            return 0xFF;
        }
    }

    return Bus<Impl>::template flushReadRequest<Dev>();
}

template<typename Impl>
template<Device::Type Dev>
void VideoBus<Impl>::flushWriteRequest(uint8_t value) {
    check(test_bit<Bus<Impl>::template W<Dev>>(this->requests));

    if constexpr (Dev == Device::Cpu) {
        if (isAcquired()) {
            // CPU fails to write while bus is acquired either by PPU or DMA.
            reset_bit<Bus<Impl>::template W<Dev>>(this->requests);
            return;
        }
    }

    return Bus<Impl>::template flushWriteRequest<Dev>(value);
}

template<typename Impl>
template<Device::Type Dev>
void VideoBus<Impl>::acquire() {
    set_bit<Dev>(acquirers);
}

template<typename Impl>
template<Device::Type Dev>
void VideoBus<Impl>::release() {
    reset_bit<Dev>(acquirers);
}

template<typename Impl>
bool VideoBus<Impl>::isAcquired() const {
    return acquirers;
}

template<typename Impl>
template<Device::Type Dev>
bool VideoBus<Impl>::isAcquiredBy() const {
    return test_bit<Dev>(acquirers);
}

template<typename Impl>
void VideoBus<Impl>::saveState(Parcel &parcel) const {
    parcel.writeUInt8(acquirers);
}

template<typename Impl>
void VideoBus<Impl>::loadState(Parcel &parcel) {
    acquirers = parcel.readUInt8();
}

template<typename Bus, Device::Type Dev>
void VideoBusView<Bus, Dev>::acquire() {
    this->bus.template acquire<Dev>();
}

template<typename Bus, Device::Type Dev>
void VideoBusView<Bus, Dev>::release() {
    this->bus.template release<Dev>();
}

template<typename Bus, Device::Type Dev>
template<Device::Type SomeDev>
bool VideoBusView<Bus, Dev>::isAcquiredBy() const {
    return this->bus.template isAcquiredBy<SomeDev>();
}

template<typename Bus, Device::Type Dev>
bool VideoBusView<Bus, Dev>::isAcquiredByMe() const {
    return this->bus.template isAcquiredBy<Dev>();
}