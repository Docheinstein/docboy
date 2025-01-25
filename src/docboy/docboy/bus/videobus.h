#ifndef VIDEOBUS_H
#define VIDEOBUS_H

#include "docboy/bus/bus.h"

class Parcel;

template <typename BusType, Device::Type Dev>
class VideoBusView;

template <typename Impl>
class VideoBus : public Bus {
    DEBUGGABLE_CLASS()

public:
    template <Device::Type Dev>
    using View = VideoBusView<Impl, Dev>;

    template <Device::Type Dev>
    uint8_t flush_read_request();

    template <Device::Type Dev>
    void flush_write_request(uint8_t value);

    template <Device::Type Dev>
    void acquire();

    template <Device::Type Dev>
    void release();

    bool is_acquired() const;

    template <Device::Type Dev>
    bool is_acquired_by() const;

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    uint16_t acquirers {}; // TODO: can be uint8_t if OAM BUG for IDU should not be handled in CGB
};

template <typename BusType, Device::Type Dev>
class VideoBusView : public BusView<BusType, Dev> {
public:
    /* implicit */ VideoBusView(BusType& bus) :
        BusView<BusType, Dev>(bus) {
    }

    void acquire();
    void release();

    template <Device::Type SomeDev>
    bool is_acquired_by() const;

    bool is_acquired_by_this() const;
};

#include "docboy/bus/videobus.tpp"

#endif // VIDEOBUS_H