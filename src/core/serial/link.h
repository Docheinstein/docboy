#ifndef LINK_H
#define LINK_H

#include "core/clock/clockable.h"

class SerialEndpoint;

using ISerialLink = IClockable;

class SerialLink : public ISerialLink {
public:
    class Plug {
    public:
        friend class SerialLink;
        explicit Plug(ISerialLink &link);
        ISerialLink & attach(SerialEndpoint *endpoint);
        void detach();
    private:
        ISerialLink &link;
        SerialEndpoint *endpoint;
    };


    SerialLink();
    ~SerialLink() override = default;

    void tick() override;

    Plug plug1;
    Plug plug2;
};

#endif // LINK_H