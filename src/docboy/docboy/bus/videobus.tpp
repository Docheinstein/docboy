#include "utils/bits.h"
#include "utils/parcel.h"

template<typename Impl>
template<Device::Type Dev>
uint8_t VideoBus<Impl>::flush_read_request() {
    ASSERT(test_bit<Bus<Impl>::template R<Dev>>(this->requests));

    if constexpr (Dev == Device::Cpu) {
        if (is_acquired()) {
            // CPU reads FF while bus is acquired either by PPU or DMA.
            reset_bit<Bus<Impl>::template R<Dev>>(this->requests);
            return 0xFF;
        }
    }

    return Bus<Impl>::template flush_read_request<Dev>();
}

template<typename Impl>
template<Device::Type Dev>
void VideoBus<Impl>::flush_write_request(uint8_t value) {
    ASSERT(test_bit<Bus<Impl>::template W<Dev>>(this->requests));

    if constexpr (Dev == Device::Cpu) {
        if (is_acquired()) {
            // CPU fails to write while bus is acquired either by PPU or DMA.
            reset_bit<Bus<Impl>::template W<Dev>>(this->requests);
            return;
        }
    }

    return Bus<Impl>::template flush_write_request<Dev>(value);
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
bool VideoBus<Impl>::is_acquired() const {
    return acquirers;
}

template<typename Impl>
template<Device::Type Dev>
bool VideoBus<Impl>::is_acquired_by() const {
    return test_bit<Dev>(acquirers);
}

template<typename Impl>
void VideoBus<Impl>::save_state(Parcel &parcel) const {
    Bus<Impl>::save_state(parcel);
    parcel.write_uint8(acquirers);
}

template<typename Impl>
void VideoBus<Impl>::load_state(Parcel &parcel) {
    Bus<Impl>::load_state(parcel);
    acquirers = parcel.read_uint8();
}

template <typename Impl>
void VideoBus<Impl>::reset() {
    Bus<Impl>::reset();
    acquirers = 0;
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
bool VideoBusView<Bus, Dev>::is_acquired_by() const {
    return this->bus.template is_acquired_by<SomeDev>();
}

template<typename Bus, Device::Type Dev>
bool VideoBusView<Bus, Dev>::is_acquired_by_this() const {
    return this->bus.template is_acquired_by<Dev>();
}