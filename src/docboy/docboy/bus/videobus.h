#ifndef VIDEOBUS_H
#define VIDEOBUS_H

#include "docboy/bus/bus.h"

class Parcel;

template <typename Bus, Device::Type Dev>
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
    uint8_t acquirers {};
};

template <typename Bus, Device::Type Dev>
class VideoBusView : public BusView<Bus, Dev> {
public:
    /* implicit */ VideoBusView(Bus& bus) :
        BusView<Bus, Dev>(bus) {
    }

    void acquire();
    void release();

    template <Device::Type SomeDev>
    bool is_acquired_by() const;

    bool is_acquired_by_this() const;
};

#include "docboy/bus/videobus.tpp"

#endif // VIDEOBUS_H