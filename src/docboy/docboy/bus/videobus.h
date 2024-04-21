#ifndef VIDEOBUS_H
#define VIDEOBUS_H

#include "bus.h"

class Parcel;

template <typename Bus, Device::Type Dev>
class VideoBusView;

template <typename Impl>
class VideoBus : public Bus<Impl> {
    DEBUGGABLE_CLASS()

public:
    template <Device::Type Dev>
    using View = VideoBusView<Impl, Dev>;

    template <Device::Type Dev>
    uint8_t flushReadRequest();

    template <Device::Type Dev>
    void flushWriteRequest(uint8_t value);

    template <Device::Type Dev>
    void acquire();

    template <Device::Type Dev>
    void release();

    [[nodiscard]] bool isAcquired() const;

    template <Device::Type Dev>
    [[nodiscard]] bool isAcquiredBy() const;

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

    void reset();

private:
    uint8_t acquirers {};
};

template <typename Bus, Device::Type Dev>
class VideoBusView : public BusView<Bus, Dev> {
public:
    VideoBusView(Bus& bus) :
        BusView<Bus, Dev>(bus) {
    }

    void acquire();
    void release();

    template <Device::Type SomeDev>
    [[nodiscard]] bool isAcquiredBy() const;

    [[nodiscard]] bool isAcquiredByMe() const;
};

#include "videobus.tpp"

#endif // VIDEOBUS_H