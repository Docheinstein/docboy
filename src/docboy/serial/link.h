#ifndef LINK_H
#define LINK_H

#include <cstdint>

class ISerialEndpoint;
class SerialPort;

class SerialLink {
public:
    struct Plug {
        explicit Plug(SerialLink& link);

        SerialLink& attach(ISerialEndpoint* endpoint);
        void detach();

        SerialLink& link;
        ISerialEndpoint* endpoint {};
    };

    void tick() const;

    Plug plug1 {*this};
    Plug plug2 {*this};
};

#endif // LINK_H